
#pragma once

#include <cmath>

#define feq(a, b, eps) (fabsf((a) - (b)) <= (eps))

struct Rectangle
{
    int height;
    int width;
};

Rectangle fitDimensionsToRatio(const Rectangle rec, const float targetRatio);
