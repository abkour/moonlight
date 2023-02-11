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

inline bool aabb_in_positive_halfspace_of_plane(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;
    
    return s > r;
}

inline bool aabb_intersects_positive_halfspace_of_plane(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;

    return s > r || abs(s) <= r;
}

inline bool frustum_contains_aabb(const Frustum& frustum, const AABB& aabb)
{
    if (aabb_in_positive_halfspace_of_plane(frustum.near, aabb))
    {
        return false;
    }
    if (aabb_in_positive_halfspace_of_plane(frustum.left, aabb))
    {
        return false;
    }
    if (aabb_in_positive_halfspace_of_plane(frustum.right, aabb))
    {
        return false;
    }
    if (aabb_in_positive_halfspace_of_plane(frustum.bottom, aabb))
    {
        return false;
    }
    if (aabb_in_positive_halfspace_of_plane(frustum.top, aabb))
    {
        return false;
    }
    if (aabb_in_positive_halfspace_of_plane(frustum.far, aabb))
    {
        return false;
    }

    return true;
}

}