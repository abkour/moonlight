#include "ray.hpp"

namespace moonlight {

Ray::Ray(const Vector3<float>& origin, const Vector3<float>& direction)
    : o(origin)
    , d(direction)
{
    invd.x = 1.f / direction.x;
    invd.y = 1.f / direction.y;
    invd.z = 1.f / direction.z;
}

Ray::Ray(const Ray& other)
    : o(other.o)
    , d(other.d)
    , invd(other.invd)
{}

Ray& Ray::operator=(const Ray& other) {
    o = other.o;
    d = other.d;
    invd = other.invd;
    return *this;
}

Vector3<float> Ray::operator()(const float t) {
    return o + (t * d);
}

IntersectionParams ray_hit_triangle(const Ray& ray, const Vector3<float>* tris)
{
    using Vector3f = Vector3<float>;

    IntersectionParams intersect_params;

    const float epsilon = 1e-7;
    const Vector3f e0 = tris[1] - tris[0];
    const Vector3f e1 = tris[2] - tris[0];
    const Vector3f q = cross(ray.d, e1);
    const float a = dot(e0, q);

    if (a > -epsilon && a < epsilon)
    {
        return intersect_params;
    }

    const float f = 1.f / a;
    const Vector3f s = ray.o - tris[0];
    intersect_params.u = f * dot(s, q);

    if (intersect_params.u < 0.f)
    {
        return intersect_params;
    }

    const Vector3f r = cross(s, e0);
    intersect_params.v = f * dot(ray.d, r);

    if (intersect_params.v < 0.f || 
        intersect_params.u + intersect_params.v > 1.f)
    {
        return intersect_params;
    }

    intersect_params.t = f * dot(e1, r);

    return intersect_params;
}

std::ostream& operator<<(std::ostream& os, const Ray& ray) {
    return os << "origin: " << ray.o << ", direction: " << ray.d;
}

}