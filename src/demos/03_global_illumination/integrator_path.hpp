#pragma once 
#include "integrator.hpp"
#include "material.hpp"
#include "pdf_cosine.hpp"
#include "pdf_light.hpp"
#include "pdf_mixture.hpp"

namespace moonlight
{

struct PathIntegrator : Integrator
{

    Vector3<float> integrate(Ray& ray, const Model* model, ILight* light_source, int traversal_depth) override
    {
        if (traversal_depth <= 0)
        {
            return Vector3<float>(0.f);
        }

        IntersectionParams its = model->intersect(ray);
        IntersectionParams its_light = light_source->intersect(ray);

        // The light source is hit before the scene geometry.
        if (its_light.is_intersection())
        {
            if (its_light.t < its.t)
            {
                return light_source->albedo();
            }
        }
        if (!its.is_intersection())
        {
            return Vector3<float>(0.f);
        }

        its.point = ray.o + (ray.t * ray.d);

        uint32_t material_idx = model->material_idx(its);
        IMaterial* material = model->get_material(material_idx);
        Vector3<float> attenuation = model->color_rgb(material_idx);

        CosinePDF cosine_pdf(its.normal);
        LightPDF light_pdf(light_source, its.point);
        MixturePDF mixed_pdf(&cosine_pdf, &light_pdf);

        Vector3<float> random_dir = mixed_pdf.generate();
        float pdf = mixed_pdf.value(random_dir);

        Vector3<float> n_dir = normalize(random_dir);
        Ray scattered(its.point + n_dir * 1e-3, n_dir);

        return
            attenuation *
            material->scattering_pdf(scattered, its) *
            integrate(scattered, model, light_source, traversal_depth - 1) /
            pdf;
    }

};

}