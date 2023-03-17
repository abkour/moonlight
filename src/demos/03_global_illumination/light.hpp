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

    virtual Vector3<float> sample() = 0;
    virtual IntersectionParams intersect(const Ray& ray) = 0;

    Vector3<float> albedo() const
    {
        return m_albedo;
    }

protected:

    Vector3<float> m_albedo;
};

}