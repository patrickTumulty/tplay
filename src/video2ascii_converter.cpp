
#include "video2ascii_converter.hpp"
#include "greedy_matrix.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "utils.hpp"
#include <cmath>

const char *ASCII_DENSITY_RAMP = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
const int ASCII_DENSITY_RAMP_LEN = strlen(ASCII_DENSITY_RAMP);

Video2AsciiConverter::Video2AsciiConverter() : _asciiData(std::make_unique<greedy_matrix<char>>(1, 1))
{
}

void Video2AsciiConverter::processPixelBuffer(const imatrix<pixel> &buffer)
{
    if (_videoHeight != buffer.height() || _videoWidth != buffer.width())
    {
        spdlog::info("Video buffer size change: {}x{} -> {}x{}", _videoWidth, _videoHeight, buffer.width(),
                     buffer.height());

        _videoHeight = buffer.height();
        _videoWidth = buffer.width();
        _videoRatio = _videoWidth / static_cast<float>(_videoHeight);

        spdlog::info("Resizing ascii buffer: video change");

        auto rec = fitDimensionsToRatio(_terminalSize, _videoRatio * 2.0);
        _asciiData->resize(rec.height, rec.width);

        _pixelStepWidth = _videoWidth / _asciiData->width();
        _pixelStepHeight = _videoHeight / _asciiData->height();
    }

    if (_terminalSizeChange)
    {
        spdlog::info("Resizing ascii buffer: terminal change");

        auto rec = fitDimensionsToRatio(_terminalSize, _videoRatio * 2.0);
        _asciiData->resize(rec.height, rec.width);
        _terminalSizeChange = false;
    }

    int pixelIdxX = 0;
    int pixelIdxY = 0;
    for (int i = 0; i < _asciiData->height(); i++)
    {
        for (int j = 0; j < _asciiData->width(); j++)
        {
            float luminance = averagePixelsLuminance(pixelIdxX, pixelIdxY, _pixelStepHeight, _pixelStepWidth, buffer);
            int offset = ASCII_DENSITY_RAMP_LEN * luminance;
            _asciiData->set(ASCII_DENSITY_RAMP[offset], j, i);
            pixelIdxX += _pixelStepWidth;
            pixelIdxY += _pixelStepHeight;
        }
    }
}

void Video2AsciiConverter::onTerminalUpdate()
{
}

float Video2AsciiConverter::averagePixelsLuminance(int x, int y, int height, int width, const imatrix<pixel> &buffer)
{
    float total = height * width;
    float luminanceSum = 0.0f;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            luminanceSum += buffer.get(i + y, j + x).luminance();
        }
    }
    return luminanceSum / total;
}

void Video2AsciiConverter::onTerminalSizeChange(Rectangle newSize)
{
    _terminalSize = newSize;
    _terminalSizeChange = true;
}
