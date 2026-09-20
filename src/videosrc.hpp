
#pragma once

#include "gst/gstelement.h"

class IVideoSrc
{
  public:
    virtual GstElement *getSrcElement() = 0;
};

class AbstractVideoSrc : public IVideoSrc
{
  public:
    explicit AbstractVideoSrc(GstElement *pipeline) : _pipeline(pipeline)
    {
    }

  protected:
    GstElement *_pipeline;
};
