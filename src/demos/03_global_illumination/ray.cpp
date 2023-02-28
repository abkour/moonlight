#include "ray.hpp"

namespace moonlight {

Ray::Ray(const Vector3& origin, const Vector3& direction)
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

Vector3 Ray::operator()(const float t) {
    return o + (t * d);
}

std::ostream& operator<<(std::ostream& os, const Ray& ray) {
    return os << "origin: " << ray.o << ", direction: " << ray.d;
}

}