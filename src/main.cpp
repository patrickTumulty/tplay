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
#include <memory>
#include <ncurses.h>
#include <ncursesw/ncurses.h>
#include <spdlog/spdlog.h>
#include "video_pipeline.hpp"
#include "videosrc_udp.hpp"
#include <unistd.h>

#include "logging.hpp"
#include "tui_session.hpp"
#include "utils.hpp"
#include "videosrc.hpp"

int main(int argc, char *argv[])
{
    logging::init();

    Ip ip;
    int port = 0;
    VideoSourceType videoSourceType = NONE;
    NetworkSource networkSource;

    for (int i = 1; i < argc; i++)
    {
        if (sscanf(argv[i], "udp://%hhu.%hhu.%hhu.%hhu:%d", &ip.octet0, &ip.octet1, &ip.octet2, &ip.octet3, &port))
        {
            videoSourceType = UDP_MPEGTS;
        }
        else if (sscanf(argv[i], "udp://localhost:%d", &port))
        {
            ip = Ip::localhost();
            videoSourceType = UDP_MPEGTS;
        }
    }

    spdlog::info("**** tplay: STARTING");

    std::shared_ptr<IVideoSrc> videoSource;

    switch (videoSourceType)
    {
    case UDP_MPEGTS:
        videoSource = std::make_shared<UdpVideoSrc>(ip, port, nullptr);
        spdlog::info("udp://{}.{}.{}.{}:{}", ip.octet3, ip.octet2, ip.octet1, ip.octet0, port);
        break;
    case NONE:
    default:

        return 0;
    }


    auto videoPipeline = VideoPipeline(nullptr);
    videoPipeline.start();

    auto session = TUISession();
    session.run();

    spdlog::info("**** tplay: EXITING");

    return 0;
}
