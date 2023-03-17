#pragma once
#include "light.hpp"

namespace moonlight
{

class AreaLight : public ILight
{
public:

    AreaLight() = default;

    AreaLight(  Vector3<float> albedo,
                Vector3<float> v0, 
                Vector3<float> v1, 
                Vector3<float> v2, 
                Vector3<float> v3)
        : v0(v0), v1(v1), v2(v2), v3(v3)
    {
        m_albedo = albedo;
        center = (v0 + v1 + v2 + v3) * 0.25;
    }

    Vector3<float> sample() override;

    IntersectionParams intersect(const Ray& ray) override;

private:

    Vector3<float> center;
    Vector3<float> v0, v1, v2, v3;
};

}