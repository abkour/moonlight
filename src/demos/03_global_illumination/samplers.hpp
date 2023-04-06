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

// The concentric disk is oriented along the z-axis
inline bool sample_concentrid_disk(Vector2<float>& sample)
{
    using vec2f = Vector2<float>;
    using vec3f = Vector3<float>;

    float r0 = random_in_range(0.f, 1.f);
    float r1 = random_in_range(0.f, 1.f);
    const vec2f p(r0, r1);

    vec2f off = 2.f * p - vec2f(1.f);
    if (off.x == 0 && off.y == 0) 
        return false;

    float theta, r;
    if (std::abs(off.x) > std::abs(off.y)) {
        r = off.x;
        theta = (ML_PI / 4) * (off.y / off.x);
    } else {
        r = off.y;
        theta = (ML_PI / 2) - (ML_PI / 4) * (off.x / off.y);
    }


    sample.x = r * std::cos(theta);
    sample.y = r * std::sin(theta);
    return true;
}

}