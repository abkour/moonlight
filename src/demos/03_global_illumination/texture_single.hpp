#pragma once
#include "texture.hpp"

namespace moonlight
{

class SingleColor : public ITexture
{
public:
    
    SingleColor(float r, float g, float b, float a)
        : ITexture(r, g, b, a)
    {}

    SingleColor(const Vector4<float>& other)
        : ITexture(other)
    {}

    Vector4<float> color(float u, float v) override
    {
        return m_color;
    }
};

}