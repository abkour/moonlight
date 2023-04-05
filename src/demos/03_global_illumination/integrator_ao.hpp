#pragma once
#include "coordinate_system.hpp"
#include "integrator.hpp"
#include "samplers.hpp"
#include "../../utility/random_number.hpp"

namespace moonlight
{

struct AOIntegrator : public Integrator
{
    AOIntegrator(float visibility_scale)
        : visibility_scale(visibility_scale)
        , ambient_color(1.f)
    {}

    AOIntegrator(const Vector3<float>& ambient_color, float visibility_scale)
        : visibility_scale(visibility_scale)
        , ambient_color(ambient_color)
    {}

    Vector3<float> integrate(Ray& ray, const Model* model, ILight* light_source, int traversal_depth) override
    {
        auto its = model->intersect(ray);

        if (!its.is_intersection())
            return Vector3<float>(0.f, 0.f, 0.f);

        CoordinateSystem cs(its.normal);

        its.point = ray.o + ray.t * ray.d;
        
        auto sample_dir = cs.to_local(random_cosine_direction());
        sample_dir = normalize(sample_dir);
        Ray random_ray(its.point + sample_dir * 1e-5, sample_dir);

        IntersectionParams ao_its = model->intersect(random_ray);
        if (ao_its.is_intersection() && ao_its.t < visibility_scale)
        {
            Vector3<float> result(ambient_color);
            result *= dot(its.normal, sample_dir) / ML_PI;
            return result;
        }

        return ambient_color;
    }

    Vector3<float> ambient_color;
    float visibility_scale;
};

}