#include <cstdint>
#include <cstdio>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/gstbus.h>
#include <gst/gstelement.h>
#include <gst/gstelementfactory.h>
#include <gst/gstmessage.h>
#include <gst/gstpad.h>
#include <gst/gstsample.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video.h>
#include <ncurses.h>
#include <ncursesw/ncurses.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include "greedy_matrix.hpp"
#include "logging.hpp"
#include "tui_session.hpp"
#include "utils.hpp"

struct Ip
{
    uint8_t octet3{};
    uint8_t octet2{};
    uint8_t octet1{};
    uint8_t octet0{};

    constexpr uint32_t address() const
    {
        return (static_cast<uint32_t>(octet3) << 24) | (static_cast<uint32_t>(octet2) << 16) |
               (static_cast<uint32_t>(octet1) << 8) | static_cast<uint32_t>(octet0);
    }

    static Ip localhost()
    {
        return {127, 0, 0, 1};
    }

    std::string toStr()
    {
        char ipStr[16];
        (void)snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", octet3, octet2, octet1, octet0);
        return std::string(ipStr);
    }
};

enum VideoSource : uint8_t
{
    NONE = 0,
    UDP_MPEGTS = 1
};

struct NetworkSource
{
    Ip ip{};
    int port;
};

struct PipelineContext
{
    bool resolutionSet = false;
    int pixelWidth;
    int pixelHeight;
    VideoSource videoSource = NONE;
    union {
        NetworkSource network{};
    };
    GstElement *pipeline;
    GstElement *h265parse;
};

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

static void onPadAdded(GstElement *_, GstPad *newPad, gpointer userData)
{
    PipelineContext *context = (PipelineContext *)userData;

    GstPad *sinkPad = gst_element_get_static_pad(context->h265parse, "sink");
    if (gst_pad_is_linked(sinkPad))
    {
        spdlog::warn("Unable to link new pad");
        gst_object_unref(sinkPad);
        return;
    }

    GstCaps *caps = gst_pad_get_current_caps(newPad);

    if (!caps)
    {
        caps = gst_pad_query_caps(newPad, NULL);
    }

    if (caps)
    {
        const GstStructure *structure = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(structure);

        spdlog::info("New pad: {}", name);

        if (g_str_has_prefix(name, "video/x-h265"))
        {
            GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
            if (GST_PAD_LINK_FAILED(ret))
            {
                spdlog::error("Failed to link demux -> parser: {}", gst_pad_link_get_name(ret));
            }
        }
        gst_caps_unref(caps);
    }

    gst_object_unref(sinkPad);
}

#define RETURN_IF_NULL(VAR)                                                                                            \
    if (!(VAR))                                                                                                        \
    {                                                                                                                  \
        spdlog::error("Unable to initialize {}", #VAR);                                                                \
        return nullptr;                                                                                                \
    }

GstElement *createUdpSource(PipelineContext *context, Ip ip, int port)
{
    if (!context | !context->pipeline)
        return nullptr;

    GstElement *source = gst_element_factory_make("udpsrc", "source");
    RETURN_IF_NULL(source);
    GstElement *demux = gst_element_factory_make("tsdemux", "demux");
    RETURN_IF_NULL(demux);
    GstElement *parser = gst_element_factory_make("h265parse", "parser");
    RETURN_IF_NULL(parser);
    GstElement *decoder = gst_element_factory_make("nvh265dec", "decoder");
    RETURN_IF_NULL(decoder);
    GstElement *converter = gst_element_factory_make("videoconvert", "converter");
    RETURN_IF_NULL(converter);
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "udp-source-filter");
    RETURN_IF_NULL(capsfilter);

    context->h265parse = parser;

    g_object_set(source,                        //
                 "port", port,                  //
                 "address", ip.toStr().c_str(), //
                 "auto-multicast", true,        //
                 NULL);

    GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(context->pipeline), //
                     source,                     //
                     demux,                      //
                     parser,                     //
                     decoder,                    //
                     converter,                  //
                     capsfilter,                 //
                     NULL);

    if (!gst_element_link(source, demux))
    {
        spdlog::error("Failed to link source -> demux");
        return nullptr;
    }

    if (!gst_element_link_many(parser,     //
                               decoder,    //
                               converter,  //
                               capsfilter, //
                               NULL))
    {

        spdlog::error("Failed to link parser -> decoder -> converter -> sink");
        return nullptr;
    }

    g_signal_connect(demux, "pad-added", G_CALLBACK(onPadAdded), context);

    return capsfilter;
}

#pragma pack(push, 1)
struct Pixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    float brightness()
    {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }
};
#pragma pack(pop)

Pixel *getPixel(uint8_t *buffer, uint32_t x, uint32_t y, uint32_t maxX, uint32_t maxY)
{
    return nullptr;
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

        gst_structure_get_int(s, "width", &context->pixelWidth);
        gst_structure_get_int(s, "height", &context->pixelHeight);

        context->resolutionSet = true;

        GstVideoMeta *meta = gst_buffer_get_video_meta(buffer);

        int stride = meta ? meta->stride[0] : context->pixelWidth * 3;

        const gchar *format = gst_structure_get_string(s, "format");

        spdlog::info("Resolution {}x{} stride {} '{}'", context->pixelWidth, context->pixelHeight, stride, format);
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

void runPipeline(PipelineContext *context)
{
    gst_init(nullptr, nullptr);

    gst_debug_set_default_threshold(GST_LEVEL_INFO);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    gst_debug_remove_log_function(gst_debug_log_default);
    gst_debug_add_log_function(gstLogToSpdlog, nullptr, nullptr);
#pragma GCC diagnostic pop

    // Create the elements
    GstElement *pipeline = gst_pipeline_new("tplay-pipeline");
    context->pipeline = pipeline;
    GstElement *source = createUdpSource(context, context->network.ip, context->network.port);
    GstElement *appsink = gst_element_factory_make("appsink", "appsink");

    if (!appsink)
    {
        spdlog::error("Failed to create appsink");
        return;
    }

    g_object_set(appsink, "emit-signals", TRUE, "sync", FALSE, NULL);

    g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSample), &context);

    // Check if elements were created successfully
    if (!pipeline || !source)
    {
        spdlog::error("Failed to create elements: pipeline={} source={} appsink={}", (void *)pipeline, (void *)source,
                      (void *)appsink);
        return;
    }

    // Build the pipeline by adding elements and linking them
    spdlog::debug("Adding source and appsink to pipeline");
    gst_bin_add_many(GST_BIN(context->pipeline), appsink, NULL);
    if (gst_element_link(source, appsink) != TRUE)
    {
        spdlog::error("Elements could not be linked.");
        gst_object_unref(pipeline);
        return;
    }

    // Set the pipeline to the PLAYING state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait for 3 seconds to let it run
    spdlog::info("Pipeline running...");
    sleep(10);

    // Tear down and clean up
    spdlog::info("Stopping pipeline...");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Unreference the pipeline to free all internal elements and memory
    gst_object_unref(pipeline);
}

struct CliContext
{
    int rows;
    int cols;
};

// void updatePresentationWindow(CliContext &ctx)
// {
//     Rectangle videoFrame = {
//         .height = ctx.rows - 1,
//         .width = ctx.cols - 1,
//     };
//
//     auto rec = fitDimensionsToRatio(videoFrame, 32.0f / 9.0f);
//     int offsetX = std::max(1, (videoFrame.width - rec.width) / 2);
//     int offsetY = std::max(0, (videoFrame.height - rec.height) / 2);
//
//     drawBox(offsetX, offsetY, rec.height, rec.width);
// }

// void onTerminalSizeChange(Rectangle newSize)
// {
// }
//
// void runUIThread()
// {
//     initscr();
//     noecho();
//     cbreak();
//     keypad(stdscr, TRUE);
//     nodelay(stdscr, TRUE);
//     curs_set(0);
//
//     CliContext ctx{};
//
//     getmaxyx(stdscr, ctx.rows, ctx.cols);
//
//     while (true)
//     {
//         clear();
//
//         updatePresentationWindow(ctx);
//
//         refresh();
//
//         int ch = getch();
//         if (ch == KEY_RESIZE)
//         {
//             Rectangle rec{};
//             getmaxyx(stdscr, rec.height, rec.width);
//             onTerminalSizeChange(rec);
//         }
//         else if (ch == 27) // ESC
//         {
//             break;
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(33));
//     }
//
//     endwin();
// }

int main(int argc, char *argv[])
{
    logging::init();

    Ip ip;
    int port = 0;
    VideoSource videoSource = NONE;
    PipelineContext context;

    for (int i = 1; i < argc; i++)
    {
        if (sscanf(argv[i], "udp://%hhu.%hhu.%hhu.%hhu:%d", &ip.octet0, &ip.octet1, &ip.octet2, &ip.octet3, &port))
        {
            videoSource = UDP_MPEGTS;
            context.network.ip = ip;
            context.network.port = port;
        }
        else if (sscanf(argv[i], "udp://localhost:%d", &port))
        {
            ip = Ip::localhost();
            videoSource = UDP_MPEGTS;
            context.network.ip = ip;
            context.network.port = port;
        }
    }

    spdlog::info("**** tplay: STARTING");

    // switch (videoSource)
    // {
    // case UDP_MPEGTS:
    //     spdlog::info("udp://{}.{}.{}.{}:{}", ip.octet3, ip.octet2, ip.octet1, ip.octet0, port);
    //     break;
    // case NONE:
    // default:
    //
    //     return 0;
    // }


    auto session = TUISession();
    session.run();

    spdlog::info("**** tplay: EXITING");

    return 0;
}
