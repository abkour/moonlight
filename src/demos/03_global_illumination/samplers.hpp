#pragma once
#include "../../simple_math.hpp"
#include "../../utility/random_number.hpp"

#define PIQuarter 0.785398163f
#define PIHalf PIQuarter * 2.f

namespace moonlight
{

inline Vector2<float> sample_triangle(const Vector2<float>& r)
{
    float sq = std::sqrt(r[0]);
    return Vector2<float>(1 - sq, r[1] * sq);
}

}