#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

inline Vector2<float> sample_triangle(const Vector2<float>& r)
{
    float sq = std::sqrt(r[0]);
    return Vector2<float>(1 - sq, r[1] * sq);
}

}