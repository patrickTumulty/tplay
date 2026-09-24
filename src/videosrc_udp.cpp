
#include "videosrc_udp.hpp"
#include "gst/gstbin.h"
#include "gst/gstelement.h"
#include "gst/gstpad.h"
#include "gst/gstutils.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <stdexcept>

namespace
{

enum VideoCodec : uint8_t
{
    NONE = 0,
    H264,
    H265
};

void linkNewH26xPad(UdpVideoSrcContext *context, GstPad *newPad, VideoCodec codec)
{
    GstElement *h26xsink = codec == VideoCodec::H264 ? context->h264sink : context->h265sink;
    GstElement *h26xsrc = codec == VideoCodec::H264 ? context->h264src : context->h265src;

    GstPad *sinkPad = gst_element_get_static_pad(h26xsink, "sink");
    if (gst_pad_is_linked(sinkPad))
    {
        spdlog::warn("Unable to link new pad");
        gst_object_unref(sinkPad);
        return;
    }

    GstPadLinkReturn ret = gst_pad_link(newPad, sinkPad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        spdlog::error("Failed to link demux -> parser: {}", gst_pad_link_get_name(ret));
    }

    if (!gst_element_link(h26xsrc, context->h26xsink))
    {
        spdlog::error("Failed to link h26x src");
    }

    context->linked = true;

    gst_object_unref(sinkPad);
}

void onPadAdded(GstElement *_, GstPad *newPad, gpointer userData)
{
    UdpVideoSrcContext *context = (UdpVideoSrcContext *)userData;

    if (context->linked)
    {
        spdlog::warn("Unable to link new pad: already linked");
        return;
    }

    GstCaps *caps = gst_pad_get_current_caps(newPad);

    if (!caps)
    {
        caps = gst_pad_query_caps(newPad, NULL);
    }

    if (!caps)
    {
        spdlog::error("Unable to read pad caps");
        return;
    }

    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(structure);

    spdlog::info("New pad: {}", name);

    VideoCodec codec = VideoCodec::NONE;

    if (g_str_has_prefix(name, "video/x-h265"))
    {
        codec = VideoCodec::H265;
    }
    if (g_str_has_prefix(name, "video/x-h264"))
    {
        codec = VideoCodec::H264;
    }
    else
    {
        spdlog::error("Unsupported pad type {}", name);
    }

    if (codec != VideoCodec::NONE)
    {
        linkNewH26xPad(context, newPad, codec);
    }

    gst_caps_unref(caps);
}
} // namespace

UdpVideoSrc::UdpVideoSrc(Ip ip, int port)
{
    std::string failMessage = "Unable to initialize UDP video source";

    _srcBin = gst_bin_new("video_src_bin");
    throwIfNull(_srcBin, STR(_srcBin), failMessage);

    GstElement *source = gst_element_factory_make("udpsrc", "source");
    throwIfNull(source, STR(source), failMessage);

    GstElement *demux = gst_element_factory_make("tsdemux", "demux");
    throwIfNull(demux, STR(demux), failMessage);

    GstElement *h265parse = gst_element_factory_make("h265parse", nullptr);
    throwIfNull(h265parse, STR(parser), failMessage);

    GstElement *h265decoder = gst_element_factory_make("nvh265dec", nullptr);
    throwIfNull(h265decoder, STR(decoder), failMessage);

    GstElement *h264parse = gst_element_factory_make("h264parse", nullptr);
    throwIfNull(h264parse, STR(parser), failMessage);

    GstElement *h264decoder = gst_element_factory_make("nvh264dec", nullptr);
    throwIfNull(h264decoder, STR(decoder), failMessage);

    GstElement *converter = gst_element_factory_make("videoconvert", "converter");
    throwIfNull(converter, STR(converter), failMessage);

    GstElement *capsfilter = gst_element_factory_make("capsfilter", "udp-source-filter");
    throwIfNull(capsfilter, STR(capsfilter), failMessage);
    _srcElement = capsfilter;

    _srcContext.h265sink = h265parse;
    _srcContext.h265src = h265decoder;
    _srcContext.h264sink = h264parse;
    _srcContext.h264src = h264decoder;
    _srcContext.h26xsink = converter;

    g_object_set(source,                        //
                 "port", port,                  //
                 "address", ip.toStr().c_str(), //
                 "auto-multicast", true,        //
                 NULL);

    GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGB", NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(_srcBin), //
                     source,           //
                     demux,            //
                     h265parse,        //
                     h265decoder,      //
                     h264parse,        //
                     h264decoder,      //
                     converter,        //
                     capsfilter,       //
                     NULL);

    if (!gst_element_link(source, demux))
    {
        throw std::runtime_error(std::format("{}: unable to link source -> demux", failMessage));
    }

    if (!gst_element_link_many(h265parse,   //
                               h265decoder, //
                               NULL))
    {
        throw std::runtime_error(std::format("{}: Failed to link h265 elements", failMessage));
    }

    if (!gst_element_link_many(h264parse,   //
                               h264decoder, //
                               NULL))
    {
        throw std::runtime_error(std::format("{}: Failed to link h264 elements", failMessage));
    }

    if (!gst_element_link_many(converter,  //
                               capsfilter, //
                               NULL))
    {
        throw std::runtime_error(std::format("{}: Failed to link converter -> sink", failMessage));
    }

    g_signal_connect(demux, "pad-added", G_CALLBACK(onPadAdded), &_srcContext);
}
