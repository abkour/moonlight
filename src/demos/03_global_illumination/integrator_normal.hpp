#pragma once
#include "integrator.hpp"

namespace moonlight
{

struct NormalIntegrator : Integrator
{

    /*
    *  TODO:
    *  Look into the cornell-box left wall. One triangle appears more red than the other.
    *  Is this because of the model?
    *  Or is it a flaw in the way the normalis calculated?
    */
    Vector3<float> integrate(
        Ray& ray, 
        const Model* model,
        std::vector<std::shared_ptr<ILight>>& light_sources, 
        int traversal_depth) override
    {
        IntersectionParams its = model->intersect(ray);
        if (its.is_intersection())
        {
            Vector3<float> normal_color = its.normal + Vector3<float>(1.f);
            normal_color *= 0.5f;
            return normal_color;
        }

        return Vector3<float>(0.f);
    }

};

}