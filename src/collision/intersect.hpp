#pragma once
#include <cstdint>
#include <limits>

namespace moonlight
{

struct IntersectionParams
{
    bool is_intersection() const
    {
        return t != std::numeric_limits<float>::max() && t > 0.f;
    }
    float t = std::numeric_limits<float>::max();     // Ray's t scale
    float u, v;         // Reserved for now (relevant for textured surfaces)
    uint32_t triangle_idx = 0;
};

}