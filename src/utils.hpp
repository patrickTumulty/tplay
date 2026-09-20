
#pragma once

#include <cmath>
#include <ncurses.h>

#define feq(a, b, eps) (fabsf((a) - (b)) <= (eps))

struct Rectangle
{
    int height;
    int width;
};

void drawBox(int x, int y, int height, int width);
Rectangle fitDimensionsToRatio(const Rectangle rec, const float targetRatio);

