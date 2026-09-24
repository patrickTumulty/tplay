
#pragma once

#include "gst/gstelement.h"
#include "utils.hpp"
#include "videosrc.hpp"

struct UdpVideoSrcContext
{
    bool linked = false;
    GstElement *h265sink;
    GstElement *h265src;
    GstElement *h264sink;
    GstElement *h264src;
    GstElement *h26xsink;
};

class UdpVideoSrc : public IVideoSrc
{
  public:
    explicit UdpVideoSrc(Ip ip, int port);

    GstElement *getSrcElement() const override
    {
        return _srcElement;
    }

    GstElement *getSrcBin() const override
    {
        return _srcBin;
    }

  private:
    UdpVideoSrcContext _srcContext;
    GstElement *_srcElement;
    GstElement *_srcBin;
};
