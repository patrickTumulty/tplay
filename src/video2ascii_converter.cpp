
#include "video2ascii_converter.hpp"
#include "greedy_matrix.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "utils.hpp"
#include <cmath>
#include <ncurses.h>

const char *ASCII_DENSITY_RAMP = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. ";
const int ASCII_DENSITY_RAMP_LEN = strlen(ASCII_DENSITY_RAMP);
const float LUMINANCE_GAMMA = 2.2f;

Video2AsciiConverter::Video2AsciiConverter() : _asciiData(std::make_unique<greedy_matrix<char>>(1, 1))
{
}

void Video2AsciiConverter::processPixelBuffer(const imatrix<pixel> &buffer)
{
    bool resized = false;
    if (_videoHeight != buffer.height() || _videoWidth != buffer.width())
    {
        spdlog::info("Video buffer size change: {}x{} -> {}x{}", _videoWidth, _videoHeight, buffer.width(),
                     buffer.height());

        _videoHeight = buffer.height();
        _videoWidth = buffer.width();
        _videoRatio = _videoWidth / static_cast<float>(_videoHeight);

        auto rec = fitDimensionsToRatio(_terminalSize, _videoRatio * 2.0);
        int height = _asciiData->height();
        int width = _asciiData->width();
        _asciiData->resize(rec.height, rec.width);
        resized = true;
        spdlog::info("Resizing ascii buffer: video change {}x{} -> {}x{}", width, height, _asciiData->width(),
                     _asciiData->height());
    }

    if (_terminalSizeChange)
    {
        auto rec = fitDimensionsToRatio(_terminalSize, _videoRatio * 2.0);
        int height = _asciiData->height();
        int width = _asciiData->width();
        _asciiData->resize(rec.height, rec.width);
        _terminalSizeChange = false;
        resized = true;
        spdlog::info("Resizing ascii buffer: terminal change {}x{} -> {}x{}", width, height, _asciiData->width(),
                     _asciiData->height());
    }

    if (resized && _asciiData->width() > 0 && _asciiData->height() > 0)
    {
        _pixelStepWidth = _videoWidth / _asciiData->width();
        _pixelStepHeight = _videoHeight / _asciiData->height();
    }

    for (int i = 0; i < _asciiData->height(); i++)
    {
        int pixelIdxX = 0;
        int pixelIdxY = i * _pixelStepHeight;
        for (int j = 0; j < _asciiData->width(); j++)
        {
            float luminance = averagePixelsLuminance(pixelIdxX, pixelIdxY, _pixelStepHeight, _pixelStepWidth, buffer);
            luminance = std::pow(luminance, 1.0f / LUMINANCE_GAMMA); // gamma: spread mid-tones across ramp
            int offset = std::min(ASCII_DENSITY_RAMP_LEN - 1, static_cast<int>(ASCII_DENSITY_RAMP_LEN * luminance));
            _asciiData->set(ASCII_DENSITY_RAMP[offset], j, i);
            pixelIdxX += _pixelStepWidth;
        }
    }
}

void Video2AsciiConverter::onTerminalUpdate()
{
    int offsetX = std::max(1, (_terminalSize.width - _asciiData->width()) / 2);
    int offsetY = std::max(0, (_terminalSize.height - _asciiData->height()) / 2);

    for (int i = 0; i < _asciiData->height(); i++)
    {
        for (int j = 0; j < _asciiData->width(); j++)
        {
            mvaddch(i + offsetY, j + offsetX, _asciiData->get(j, i));
        }
    }

    // drawBox(offsetX, offsetY, _asciiData->height(), _asciiData->width());
}

float Video2AsciiConverter::averagePixelsLuminance(int x, int y, int height, int width, const imatrix<pixel> &buffer)
{
    float total = height * width;
    if (total <= 0.0f)
    {
        return 0.0f;
    }
    float luminanceSum = 0.0f;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            luminanceSum += buffer.get(j + x, i + y).luminance();
        }
    }
    return luminanceSum / total;
}

void Video2AsciiConverter::onTerminalSizeChange(Rectangle newSize)
{
    _terminalSize = newSize;
    _terminalSize.height--;
    _terminalSize.width--;
    spdlog::info("Terminal size change h={} w={}", newSize.height, newSize.width);
    _terminalSizeChange = true;
}
