
#include "utils.hpp"
#include <cmath>

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
