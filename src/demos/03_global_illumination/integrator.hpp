#pragma once
#include "light.hpp"
#include "model.hpp"
#include "../../collision/ray.hpp"


namespace moonlight
{

struct Integrator
{

    virtual Vector3<float> integrate(
        Ray& ray, 
        const Model* model, 
        std::vector<std::shared_ptr<ILight>>& light_sources, 
        int traversal_depth = 0
    ) = 0;

};

}