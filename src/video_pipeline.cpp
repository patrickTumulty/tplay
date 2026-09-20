
#include "video_pipeline.hpp"
#include "gst/app/gstappsink.h"
#include "gst/gstinfo.h"
#include "gst/gstpad.h"
#include "gst/gstutils.h"
#include "gst/video/gstvideometa.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "video2ascii_converter.hpp"
#include "videosrc.hpp"
#include <stdexcept>

static void gstLogToSpdlog(GstDebugCategory *category, GstDebugLevel level, const gchar *file, const gchar *_function,
                           gint line, GObject *object, GstDebugMessage *message, gpointer userData)
{
    (void)userData;
    auto logger = spdlog::get("gst");

    const gchar *text = gst_debug_message_get(message);
    const gchar *name = category ? gst_debug_category_get_name(category) : "unknown";

    spdlog::source_loc loc{file == nullptr ? "" : file, static_cast<int>(line), _function == nullptr ? "" : _function};

    switch (level)
    {
    case GST_LEVEL_ERROR:
        logger->log(loc, spdlog::level::err, "[{}] {}", name, text);
        break;
    case GST_LEVEL_WARNING:
    case GST_LEVEL_FIXME:
        logger->log(loc, spdlog::level::warn, "[{}] {}", name, text);
        break;
    case GST_LEVEL_INFO:
        logger->log(loc, spdlog::level::info, "[{}] {}", name, text);
        break;
    case GST_LEVEL_DEBUG:
        logger->log(loc, spdlog::level::debug, "[{}] {}", name, text);
        break;
    case GST_LEVEL_LOG:
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
    default:
        logger->log(loc, spdlog::level::trace, "[{}] {}", name, text);
        break;
    }

    (void)object;
}

#define RETURN_IF_NULL(VAR)                                                                                            \
    if (!(VAR))                                                                                                        \
    {                                                                                                                  \
        spdlog::error("Unable to initialize {}", #VAR);                                                                \
        return nullptr;                                                                                                \
    }

static GstFlowReturn onNewSample(GstElement *sink, gpointer userData)
{
    PipelineContext *context = (PipelineContext *)userData;

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));

    if (!sample)
        return GST_FLOW_ERROR;

    GstMapInfo map;
    GstBuffer *buffer = gst_sample_get_buffer(sample);

    if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    if (!context->resolutionSet)
    {
        GstCaps *caps = gst_sample_get_caps(sample);
        GstStructure *s = gst_caps_get_structure(caps, 0);

        gst_structure_get_int(s, "width", &context->videoSize.width);
        gst_structure_get_int(s, "height", &context->videoSize.height);

        context->resolutionSet = true;

        auto videoSize = context->videoSize;

        context->pixelBuffer.resize(videoSize.height, videoSize.width);

        GstVideoMeta *meta = gst_buffer_get_video_meta(buffer);

        int stride = meta ? meta->stride[0] : videoSize.width * 3;

        const gchar *format = gst_structure_get_string(s, "format");

        spdlog::info("Resolution {}x{} stride {} '{}'", videoSize.width, videoSize.height, stride, format);
    }

    if (gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        auto videoSize = context->videoSize;

        pixel *pixelArray = (pixel *)map.data;
        int pixelArrayLen = map.size / 3;

        int row = 0;
        int col = 0;
        for (int i = 0; i < pixelArrayLen; i++)
        {
            if (i % videoSize.width)
            {
                row++;
                col = 0;
            }
            context->pixelBuffer.set(pixelArray[i], col++, row);
        }

        // TODO: Pass buffer to video converter

        // Always unmap what you mapped
        gst_buffer_unmap(buffer, &map);
    }

    /*
     * map.data points to the pixel data.
     * map.size is the size in bytes.
     */
    // const uint8_t *pixels = map.data;
    size_t size = map.size;

    spdlog::debug("Got frame: {} bytes", size);

    gst_buffer_unmap(buffer, &map);

    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

VideoPipeline::VideoPipeline(std::shared_ptr<IVideoSrc> videoSrc)
{
    gst_init(nullptr, nullptr);

    gst_debug_set_default_threshold(GST_LEVEL_INFO);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    gst_debug_remove_log_function(gst_debug_log_default);
    gst_debug_add_log_function(gstLogToSpdlog, nullptr, nullptr);
#pragma GCC diagnostic pop

    std::string failMessage = "Unable to init video pipeline";

    _context.pipeline = gst_pipeline_new("tplay-pipeline");
    throwIfNull(_context.pipeline, STR(_context.pipeline), failMessage);

    _context.appsink = gst_element_factory_make("appsink", "appsink");
    throwIfNull(_context.appsink, STR(_context.appsink), failMessage);

    g_object_set(_context.appsink,     //
                 "emit-signals", TRUE, //
                 "sync", FALSE,        //
                 NULL);

    g_signal_connect(_context.appsink, "new-sample", G_CALLBACK(onNewSample), &_context);

    spdlog::debug("Adding source and appsink to _pipeline");
    gst_bin_add_many(GST_BIN(_context.pipeline), _context.appsink, NULL);
    if (gst_element_link(videoSrc->getSrcElement(), _context.appsink) != TRUE)
    {
        gst_object_unref(_context.pipeline);
        gst_object_unref(_context.appsink);
        throw std::runtime_error(std::format("{}: failed to link elements", failMessage));
    }
}

VideoPipeline::~VideoPipeline()
{
}

void VideoPipeline::start()
{
    gst_element_set_state(_context.pipeline, GST_STATE_PLAYING);
}

void VideoPipeline::stop()
{
    // Tear down and clean up
    spdlog::info("Stopping _pipeline...");
    gst_element_set_state(_context.pipeline, GST_STATE_NULL);

    // Unreference the _pipeline to free all internal elements and memory
    gst_object_unref(_context.pipeline);
}
