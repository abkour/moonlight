#pragma once
#include "simple_math.hpp"

namespace moonlight
{

struct AABB
{
    Vector3 bmin, bmax;
};

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