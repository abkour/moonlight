#pragma once
#include "core/key_state.hpp"
#include "simple_math.hpp"
#include <Windows.h>
#include <DirectXMath.h>

namespace moonlight
{

struct Camera
{
    Camera(
        DirectX::XMFLOAT3 eye_position, 
        DirectX::XMFLOAT3 eye_direction,
        const float moveemnt_speed
    );

    void rotate(float dx, float dy);
    void translate(KeyState keys, float delta_time);
    void translate(DirectX::XMVECTOR pos_offset);
    void translate_around_center(KeyState keys, DirectX::XMVECTOR center, float delta_time);

    DirectX::XMFLOAT3 get_direction() const
    {
        return m_eye_direction;
    }

    DirectX::XMFLOAT3 get_position() const
    {
        return m_eye_position;
    }

    void set_movement_speed(const float movement_speed);

    DirectX::XMMATRIX view;

private:

    float m_movement_speed;
    float m_pitch, m_yaw;
    DirectX::XMFLOAT3 m_eye_position, m_eye_direction;
};

}