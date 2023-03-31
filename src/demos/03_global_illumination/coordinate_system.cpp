#include "coordinate_system.hpp"

namespace moonlight
{

using vec3f = Vector3<float>;

CoordinateSystem::CoordinateSystem(const vec3f& normal)
{
    n = normal;
    Vector3<float> a = vec3f(0.f);
    if (std::abs(normal.x) > 0.9)
    {
        a = vec3f(0.f, 1.f, 0.f);
    }
    else
    {
        a = vec3f(1.f, 0.f, 0.f);
    }
    nb = normalize(cross(n, a));
    nt = cross(n, nb);
}

vec3f CoordinateSystem::to_local(const vec3f& p) const
{
    return p.x * nt + p.y * nb + p.z * n;
}

}