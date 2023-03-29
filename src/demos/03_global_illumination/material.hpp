#pragma once
#include "texture.hpp"
#include "../../simple_math.hpp"
#include "../../collision/intersect.hpp"
#include "../../collision/ray.hpp"

namespace moonlight
{

class IMaterial
{
public:
    
    IMaterial() = default;

    IMaterial(ITexture* texture)
        : m_texture(texture)
    {
    }

    IMaterial(std::shared_ptr<ITexture>& texture)
        : m_texture(texture.get())
    {
    }

    virtual void scatter(Ray& r_out, float& pdf, const Ray& r_in, IntersectionParams& intersect) = 0;

    virtual float scattering_pdf(const Ray& scattered, IntersectionParams& intersect) = 0;

private:

    ITexture* m_texture;
};

}