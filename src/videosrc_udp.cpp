
#include "videosrc_udp.hpp"
#include "gst/gstbin.h"
#include "gst/gstelement.h"
#include "gst/gstutils.h"
#include "spdlog/spdlog.h"
#include <stdexcept>

namespace
{
void onPadAdded(GstElement *_, GstPad *newPad, gpointer userData)
{
    UdpVideoSrcContext *context = (UdpVideoSrcContext *)userData;

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
        else
        {
            spdlog::error("Unsupported pad type {}", name);
        }
        gst_caps_unref(caps);
    }

    gst_object_unref(sinkPad);
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

    GstElement *parser = gst_element_factory_make("h265parse", "parser");
    throwIfNull(parser, STR(parser), failMessage);

    GstElement *decoder = gst_element_factory_make("nvh265dec", "decoder");
    throwIfNull(parser, STR(parser), failMessage);

    GstElement *converter = gst_element_factory_make("videoconvert", "converter");
    throwIfNull(converter, STR(converter), failMessage);

    GstElement *capsfilter = gst_element_factory_make("capsfilter", "udp-source-filter");
    throwIfNull(capsfilter, STR(capsfilter), failMessage);
    _srcElement = capsfilter;

    _srcContext.h265parse = parser;

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
                     parser,           //
                     decoder,          //
                     converter,        //
                     capsfilter,       //
                     NULL);

    if (!gst_element_link(source, demux))
    {
        throw std::runtime_error(std::format("{}: unable to link source -> demux", failMessage));
    }

    if (!gst_element_link_many(parser,     //
                               decoder,    //
                               converter,  //
                               capsfilter, //
                               NULL))
    {
        throw std::runtime_error(std::format("{}: Failed to link parser -> decoder -> converter -> sink", failMessage));
    }

    g_signal_connect(demux, "pad-added", G_CALLBACK(onPadAdded), &_srcContext);
}
