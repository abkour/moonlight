#pragma once
#include <limits>

namespace moonlight
{

struct IntersectionParams
{
    bool is_intersection() const
    {
        return t != std::numeric_limits<float>::max();
    }
    float t = std::numeric_limits<float>::max();     // Ray's t scale
    float u, v;         // Reserved for now (relevant for textured surfaces)
};

}