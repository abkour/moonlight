#pragma once
#include <cstdint>
#include <limits>
#include "../simple_math.hpp"

namespace moonlight
{

struct IntersectionParams
{
    bool is_intersection() const
    {
        return t != std::numeric_limits<float>::max() && t > 0.f;
    }

    inline void set_face_normal(const Vector3<float>& incident_dir, const Vector3<float>& outward_normal)
    {
        front_face = dot(incident_dir, outward_normal) < 0.f;
        normal = front_face ? outward_normal : invert(outward_normal);
    }

    bool front_face;
    float t = std::numeric_limits<float>::max();
    float u, v;
    uint32_t triangle_idx = 0;
    Vector3<float> normal;
};

}