#include "area_light.hpp"

namespace moonlight
{

Vector3<float> AreaLight::sample()
{
    return center;
}

IntersectionParams AreaLight::intersect(const Ray& ray)
{
    IntersectionParams intersect =
        ray_hit_triangle(ray, (float*)&v0, (float*)&v1, (float*)&v2, 3);

    if (intersect.is_intersection())
    {
        return intersect;
    }

    // If the ray collides with the second triangle, t is updated.
    // Otherwise it is not, and is_intersection() will return false.
    return ray_hit_triangle(ray, (float*)&v0, (float*)&v2, (float*)&v3, 3);
    //intersect.t = intersect.t > 0.f && intersect.is_intersection() ?
    //    intersect.t : std::numeric_limits<float>::max();
}

}