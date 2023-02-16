#pragma once
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
    // The keycode is a 32-bit unsigned integer that contains a bitmask of the 
    // 4 supported keystrokes (A, D, S, W) with the respective bits (0, 1, 2, 3)
    void translate(uint32_t keycode, float delta_time);
    void translate(DirectX::XMVECTOR pos_offset);

    DirectX::XMFLOAT3 get_direction() const
    {
        return eye_direction;
    }

    DirectX::XMFLOAT3 get_position() const
    {
        return eye_position;
    }

    void set_movement_speed(const float movement_speed);

    DirectX::XMMATRIX view;

private:

    float movement_speed;
    float pitch, yaw;
    DirectX::XMFLOAT3 eye_position, eye_direction;
};

}