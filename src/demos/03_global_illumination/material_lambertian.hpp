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

    void scatter(Ray& r_out, float& pdf, const Ray& r_in, IntersectionParams& intersect) override
    {
        CoordinateSystem cs(intersect.normal);
        auto cs_dir = cs.to_local(random_cosine_direction());
        cs_dir = normalize(cs_dir);

        r_out = Ray(intersect.point + cs_dir * 1e-3, cs_dir);
        pdf = dot(cs.n, r_out.d) / ML_PI;
    }

    float scattering_pdf(const Ray& scattered, IntersectionParams& intersect) override
    {
        auto cosine = dot(intersect.normal, scattered.d);
        return cosine < 0 ? 0 : cosine / ML_PI;
    }
};

}