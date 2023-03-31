#pragma once
#include "../../simple_math.hpp"
#include "../collision/ray.hpp"

namespace moonlight
{

class ILight
{
public:

    ILight() = default;
    ILight(const Vector3<float> albedo)
        : m_albedo(albedo)
    {}

    virtual float pdf(const Vector3<float>& origin, const Vector3<float>& dir)
    {
        return 0.f;
    }

    virtual Vector3<float> sample(const Vector3<float>& origin) = 0;
    virtual IntersectionParams intersect(const Ray& ray) = 0;

    Vector3<float> albedo() const
    {
        return m_albedo;
    }

protected:

    Vector3<float> m_albedo;
};

}