
#include "utils.hpp"
#include "gst/gstelement.h"
#include <cmath>
#include <format>
#include <stdexcept>

Rectangle fitDimensionsToRatio(const Rectangle rec, const float targetRatio)
{
    Rectangle rec1 = rec;
    Rectangle rec2 = rec;

    if (rec.height == 0)
    {
        return rec;
    }

    float ratio = rec.width / static_cast<float>(rec.height);

    if (ratio > targetRatio)
    {
        for (int i = 0; i < rec.width; i++)
        {
            ratio = rec2.width / static_cast<float>(rec2.height);
            if (feq(ratio, targetRatio, 0.1f))
            {
                return rec2;
            }
            rec2.width--;
        }
    }
    else
    {
        for (int i = 0; i < rec.height; i++)
        {
            if (rec2.height == 0)
            {
                break;
            }
            ratio = rec1.width / static_cast<float>(rec1.height);
            if (feq(ratio, targetRatio, 0.1f))
            {
                return rec1;
            }
            rec1.height--;
        }
    }

    return rec;
}

void drawBox(int x, int y, int height, int width)
{
    if (!(width >= 2 && height >= 2 && x >= 0 && y >= 0 && x + width <= COLS && y + height <= LINES))
    {
        return;
    }

    mvhline(y, x + 1, ACS_HLINE, width - 2);              // Top Line
    mvhline(y + height - 1, x + 1, ACS_HLINE, width - 2); // Bottom Line

    mvvline(y + 1, x, ACS_VLINE, height - 2);             // Left Line
    mvvline(y + 1, x + width - 1, ACS_VLINE, height - 2); // Right Line

    mvaddch(y, x, ACS_ULCORNER);                          // Upper Left
    mvaddch(y, x + width - 1, ACS_URCORNER);              // Upper Right
    mvaddch(y + height - 1, x, ACS_LLCORNER);             // Lower Left
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER); // Lower Right
}

void verifyElement(GstElement *element, const char *elementName, std::string failMessage)
{
    if (element == nullptr)
    {
        throw std::runtime_error(std::format("{} : unable to create '{}'", failMessage, elementName));
    }
}
