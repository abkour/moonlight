#pragma once
#include "coordinate_system.hpp"
#include "material.hpp"
#include "../../utility/random_number.hpp"

namespace moonlight
{

struct LamberrtianMaterial : public IMaterial
{
    LamberrtianMaterial(ITexture* texture)
        : IMaterial(texture)
    {
    }

    LamberrtianMaterial(std::shared_ptr<ITexture>& texture)
        : IMaterial(texture.get())
    {
    }

    void scatter(Ray& r_out, const Ray& r_in, float& pdf, IntersectionParams& intersect) override
    {
        CoordinateSystem cs(intersect.normal);
        auto cs_dir = cs.to_local(random_cosine_direction());
        cs_dir = normalize(cs_dir);
        
        pdf = dot(intersect.normal, cs_dir) / ML_PI;
        r_out = Ray(intersect.point + cs_dir * 1e-3, cs_dir);
    }

    float scattering_pdf(const Ray& scattered, IntersectionParams& intersect) override
    {
        auto cosine = dot(intersect.normal, scattered.d);
        return cosine / ML_PI;
    }
};

}