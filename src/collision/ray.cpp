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

IntersectionParams ray_hit_triangle(
    const Ray& ray, 
    const float* tris,
    const unsigned stride)
{
    using vec3f = Vector3<float>;

    IntersectionParams intersect_params;

    const float epsilon = 1e-4;
    const vec3f e0(tris[stride]     - tris[0],
                   tris[stride + 1] - tris[1],
                   tris[stride + 2] - tris[2]
    );

    const vec3f e1(tris[stride * 2]     - tris[0],
                   tris[stride * 2 + 1] - tris[1],
                   tris[stride * 2 + 2] - tris[2]
    );

    const vec3f q = cross(ray.d, e1);
    const float a = dot(e0, q);

    if (a > -epsilon && a < epsilon)
    {
        return intersect_params;
    }

    const float f = 1.f / a;
    const vec3f s(ray.o.x - tris[0],
                  ray.o.y - tris[1],
                  ray.o.z - tris[2]
    );

    intersect_params.u = f * dot(s, q);

    if (intersect_params.u < 0.f)
    {
        return intersect_params;
    }

    const vec3f r = cross(s, e0);
    intersect_params.v = f * dot(ray.d, r);

    if (intersect_params.v < 0.f || 
        intersect_params.u + intersect_params.v > 1.f)
    {
        return intersect_params;
    }

    intersect_params.t = f * dot(e1, r);
    vec3f normal = normalize(cross(e0, e1));
    intersect_params.set_face_normal(ray.d, normal);

    return intersect_params;
}

IntersectionParams ray_hit_triangle(
    const Ray& ray,
    const float* tri0, const float* tri1, const float* tri2,
    const unsigned stride)
{
    using vec3f = Vector3<float>;

    IntersectionParams intersect_params;

    const float epsilon = 1e-7;
    const vec3f e0(
        tri1[0] - tri0[0],
        tri1[1] - tri0[1],
        tri1[2] - tri0[2]
    );

    const vec3f e1(
        tri2[0] - tri0[0],
        tri2[1] - tri0[1],
        tri2[2] - tri0[2]
    );

    const vec3f q = cross(ray.d, e1);
    const float a = dot(e0, q);

    if (a > -epsilon && a < epsilon)
    {
        return intersect_params;
    }

    const float f = 1.f / a;
    const vec3f s(
        ray.o.x - tri0[0],
        ray.o.y - tri0[1],
        ray.o.z - tri0[2]
    );

    intersect_params.u = f * dot(s, q);

    if (intersect_params.u < 0.f)
    {
        return intersect_params;
    }

    const vec3f r = cross(s, e0);
    intersect_params.v = f * dot(ray.d, r);

    if (intersect_params.v < 0.f ||
        intersect_params.u + intersect_params.v > 1.f)
    {
        return intersect_params;
    }

    intersect_params.t = f * dot(e1, r);
    vec3f normal = normalize(cross(e0, e1));
    intersect_params.set_face_normal(ray.d, normal);

    return intersect_params;
}

IntersectionParams ray_hit_triangle(
    const Ray& ray,
    const Vector3<float>& v0, const Vector3<float>& v1, const Vector3<float>& v2)
{
    using vec3f = Vector3<float>;

    IntersectionParams intersect_params;

    const float epsilon = 1e-6;
    const vec3f e0 = v1 - v0;
    const vec3f e1 = v2 - v0;

    const vec3f q = cross(ray.d, e1);
    const float a = dot(e0, q);

    if (a > -epsilon && a < epsilon)
    {
        return intersect_params;
    }

    const float f = 1.f / a;
    const vec3f s = ray.o - v0;

    intersect_params.u = f * dot(s, q);

    if (intersect_params.u < 0.f)
    {
        return intersect_params;
    }

    const vec3f r = cross(s, e0);
    intersect_params.v = f * dot(ray.d, r);

    if (intersect_params.v < 0.f ||
        intersect_params.u + intersect_params.v > 1.f)
    {
        return intersect_params;
    }

    intersect_params.t = f * dot(e1, r);
    vec3f normal = normalize(cross(e0, e1));
    intersect_params.set_face_normal(ray.d, normal);

    return intersect_params;
}

std::ostream& operator<<(std::ostream& os, const Ray& ray) {
    return os << "origin: " << ray.o << ", direction: " << ray.d;
}

}