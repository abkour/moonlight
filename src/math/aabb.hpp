#pragma once
#include "../simple_math.hpp"
#include <limits>

namespace moonlight
{

struct AABB
{
    Vector3 bmin, bmax;
};

inline AABB construct_aabb_from_points(const Vector3* points, const std::size_t n_points)
{
    constexpr float lf = std::numeric_limits<float>::min();
    constexpr float hf = std::numeric_limits<float>::max();
    AABB aabb;
    aabb.bmin = { hf, hf, hf };
    aabb.bmax = { lf, lf, lf };

    for (std::size_t i = 0; i < n_points; ++i)
    {
        if (points[i].x < aabb.bmin.x) aabb.bmin.x = points[i].x;
        if (points[i].y < aabb.bmin.y) aabb.bmin.y = points[i].y;
        if (points[i].z < aabb.bmin.z) aabb.bmin.z = points[i].z;

        if (points[i].x > aabb.bmax.x) aabb.bmax.x = points[i].x;
        if (points[i].y > aabb.bmax.y) aabb.bmax.y = points[i].y;
        if (points[i].z > aabb.bmax.z) aabb.bmax.z = points[i].z;
    }

    return aabb;
}

inline bool is_empty(const AABB& x)
{
    return x.bmin == x.bmax;
}

inline bool is_degenerate(const AABB& x)
{
    return !cwise_less(x.bmin, x.bmax);
}

inline float volume(const AABB& x)
{
    Vector3 diff = x.bmax - x.bmin;
    return diff.x * diff.y * diff.z;
}

}