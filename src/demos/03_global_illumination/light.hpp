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
    
    virtual void sample_light(const IntersectionParams& its, float* pdf, bool* visibile)
    {}

    virtual Vector3<float> normal() const
    {
        return Vector3<float>(0.f);
    }

    virtual float pdf(const Vector3<float>& origin, const Vector3<float>& dir)
    {
        return 0.f;
    }

    virtual void sample(Ray& r_out, const Ray& r_in, float& pdf, const IntersectionParams& its) = 0;
    virtual IntersectionParams intersect(const Ray& ray) = 0;

    Vector3<float> albedo() const
    {
        return m_albedo;
    }

protected:

    Vector3<float> m_albedo;
};

}