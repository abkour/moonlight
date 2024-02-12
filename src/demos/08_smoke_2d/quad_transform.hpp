#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

struct QuadTransform
{
    float m_alpha;
    float m_angle;
    float m_lifetime;
    float m_size_scale;
    Vector3<float> m_color;
    Vector2<float> m_position;
    Vector2<float> m_velocity;
};

}