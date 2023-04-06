#pragma once
#include "light.hpp"
#include "../../project_defines.hpp"

namespace moonlight
{

class PointLight : public ILight
{
public:

    PointLight(const Vector3<float>& location, const Vector3<float>& albedo)
        : ILight(albedo)
        , m_location(location)
    {}

    virtual float pdf(const Vector3<float>& origin, const Vector3<float>& dir)
    {
        Vector3<float> normalized_dir = normalize(dir);
        Ray ray(origin + normalized_dir * 1e-3, normalized_dir);
        IntersectionParams its = this->intersect(ray);

        if (!its.is_intersection())
        {
            return 0.f;
        }

        return 1.f;
    }

    virtual void sample(Ray& r_out, const Ray& r_in, float& pdf, const IntersectionParams& its) override
    {

    }

    virtual IntersectionParams intersect(const Ray& ray)
    {
        const Vector3<float> t = (m_location - ray.o) * ray.invd;

        IntersectionParams intersect_params;
        if (float_equal(t.x, t.y, 1e-3) && float_equal(t.x, t.z, 1e-3))
        {
            intersect_params.t = t.x;
        }

        return intersect_params;
    }

private:

    bool float_equal(float f0, float f1, float eps = 1e-6)
    {
        float delta = abs(f0 - f1);
        return -eps < delta && eps > delta;
    }

    Vector3<float> m_location;
};

}