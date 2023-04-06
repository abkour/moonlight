#pragma once 
#include "integrator.hpp"
#include "material.hpp"

namespace moonlight
{

struct PathIntegrator : Integrator
{
    Vector3<float> integrate(
        Ray& ray, 
        const Model* model, 
        std::vector<std::shared_ptr<ILight>>& light_sources, 
        int traversal_depth) override
    {
        if (traversal_depth <= 0)
        {
            return Vector3<float>(0.f);
        }

        IntersectionParams its = model->intersect(ray);
        IntersectionParams its_light;
        
        int light_idx = -1;
        for (int i = 0; auto& light : light_sources)
        {
            IntersectionParams its_l = light->intersect(ray);
            if (its_l.t < its_light.t)
            {
                light_idx = i;
                its_light = its_l;
            }

            ++i;
        }

        // The light source is hit before the scene geometry.
        if (its_light.is_intersection())
        {
            if (its_light.t < its.t)
            {
                return light_sources[light_idx]->albedo();
            }
        }
        if (!its.is_intersection())
        {
            return Vector3<float>(0.f);
        }

        uint32_t material_idx = model->material_idx(its);
        IMaterial* material = model->get_material(material_idx);
        Vector3<float> attenuation = model->color_rgb(material_idx);

        Ray scattered;
        float pdf = 0.f;
        
        if (random_in_range(0.f, 1.f) < 0.5f)
        {
            material->scatter(scattered, ray, pdf, its);
        }
        else
        {
            auto light = light_sources[random_in_range_int(0, light_sources.size() - 1)];
            light->sample(scattered, ray, pdf, its);
        }
        
        return
            attenuation *
            material->scattering_pdf(scattered, its) *
            integrate(scattered, model, light_sources, traversal_depth - 1) /
            pdf;
    }
};

}