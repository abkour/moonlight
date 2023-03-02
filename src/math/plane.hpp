#pragma once
#include "../simple_math.hpp"

namespace moonlight
{

struct Plane
{
    Vector3<float> normal;
    Vector3<float> point;
};

static int point_plane_intersection(const Plane& plane, const Vector3<float>& p)
{
    Vector3<float> X = normalize(p - plane.point);
    float result = dot(plane.normal, X);
    
    if (result > 0.f)
    {
        return 1;
    }
    
    return 0;
}

struct alignas(32) PlaneSIMD
{
    float nx[8];
    float ny[8];
    float nz[8];
};

}