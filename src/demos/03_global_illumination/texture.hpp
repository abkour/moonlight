#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

struct ITexture
{
    ITexture() = default;
    ITexture(float r, float g, float b, float a)
        : m_color(r, g, b, a)
    {}
    ITexture(const Vector4<float>& other)
        : m_color(other)
    {}

    virtual Vector4<float> color(float u, float v) = 0;

protected:

    Vector4<float> m_color;
};

}