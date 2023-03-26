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

    return ray_hit_triangle(ray, (float*)&v0, (float*)&v2, (float*)&v3, 3);
}

}