#pragma once
#include "texture.hpp"
#include <memory>
#include <vector>

namespace moonlight
{

class TextureImage : public ITexture
{
public:
    TextureImage() = default;
    TextureImage(std::shared_ptr<Vector4<float>[]>& other, const uint16_t image_width, const uint16_t image_height)
        : m_image(other)
        , m_image_width(image_width)
        , m_image_height(image_height)
    {}

    Vector4<float> color(float u, float v) override
    {
        uint32_t iu = u * m_image_width;
        uint32_t iv = v * m_image_height;
        uint32_t idx = iu + iv * m_image_width;
        return m_image[idx];
    }

protected:

    std::shared_ptr<Vector4<float>[]> m_image;
    const uint16_t m_image_width, m_image_height;
};

}