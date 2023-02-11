#pragma once
#include "aabb.hpp"
#include "plane.hpp"
#include "frustum.hpp"

namespace moonlight
{

// Modified version of OBB/Plane test from "Real-time Collision Detection", Christer Ericson
inline bool plane_intersects_aabb(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;

    return abs(s) <= r;
}

inline bool frustum_intersects_aabb(const Frustum& frustum, const AABB& aabb)
{
    if (plane_intersects_aabb(frustum.near, aabb))
    {
        return true;
    }
    if (plane_intersects_aabb(frustum.left, aabb))
    {
        return true;
    }
    if (plane_intersects_aabb(frustum.right, aabb))
    {
        return true;
    }
    if (plane_intersects_aabb(frustum.bottom, aabb))
    {
        return true;
    }
    if (plane_intersects_aabb(frustum.top, aabb))
    {
        return true;
    }
    if (plane_intersects_aabb(frustum.far, aabb))
    {
        return true;
    }

    return false;
}

}