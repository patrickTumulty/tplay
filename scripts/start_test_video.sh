#!/bin/bash

set -e

PORT=5000
RESOLUTION=1080

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Send a videotestsrc as H.265 MPEG-TS over UDP to localhost."
    echo
    echo "Options:"
    echo "  -p, --port PORT       UDP destination port (default: 5000)"
    echo "  -r, --resolution RES  Video resolution: 1080 or 720 (default: 1080)"
    echo "  -h, --help            Show this help"
    echo
    echo "Examples:"
    echo "  $0"
    echo "  $0 --port 15005"
    echo "  $0 --resolution 720"
    echo "  $0 -p 15005 -r 720"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -r|--resolution)
            RESOLUTION="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo
            usage
            exit 1
            ;;
    esac
done

case "$RESOLUTION" in
    1080)
        WIDTH=1920
        HEIGHT=1080
        ;;
    720)
        WIDTH=1280
        HEIGHT=720
        ;;
    *)
        echo "Error: resolution must be 1080 or 720"
        exit 1
        ;;
esac

echo "Sending ${WIDTH}x${HEIGHT} H.265 MPEG-TS to 127.0.0.1:${PORT}"

gst-launch-1.0 -v \
    videotestsrc is-live=true pattern=ball ! \
    "video/x-raw,width=${WIDTH},height=${HEIGHT},framerate=30/1" ! \
    videoconvert ! \
    x265enc tune=zerolatency \
            bitrate=5000 \
            speed-preset=ultrafast \
            option-string="repeat-headers=1" ! \
    h265parse ! \
    mpegtsmux ! \
    udpsink host=127.0.0.1 port="${PORT}"
