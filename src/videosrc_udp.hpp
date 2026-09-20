
#pragma once

#include "gst/gstelement.h"
#include "utils.hpp"
#include "videosrc.hpp"

struct UdpVideoSrcContext
{
    GstElement *h265parse;
};

class UdpVideoSrc : public AbstractVideoSrc
{
  public:
    explicit UdpVideoSrc(Ip ip, int port, GstElement *pipeline);

    GstElement *getSrcElement() override
    {
        return _srcElement;
    }

  private:
    UdpVideoSrcContext _srcContext;
    GstElement *_srcElement;
};
