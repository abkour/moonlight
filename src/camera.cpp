#include "camera.hpp"

using namespace DirectX;;

namespace moonlight
{

Camera::Camera(DirectX::XMFLOAT3 eye_position, DirectX::XMFLOAT3 eye_direction)
{
    pitch = 0.f;
    yaw = 0.f;
    this->eye_direction = eye_direction;
    this->eye_position = eye_position;

    XMVECTOR eye_position_xmv = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&eye_position));
    XMVECTOR eye_direction_xmv = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&eye_direction));
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);
}

void Camera::rotate(float dx, float dy)
{
    const float scale = 0.15f;
    yaw += dx * scale;
    pitch += dy * scale;

    if (pitch > 89.f)
    {
        pitch = 89.f;
    }
    else if (pitch < -89.f)
    {
        pitch = -89.f;
    }

    eye_direction.x = std::cos(radians(yaw)) * std::cos(radians(pitch));
    eye_direction.y = std::sin(radians(pitch));
    eye_direction.z = std::sin(radians(yaw)) * std::cos(radians(pitch));

    XMVECTOR eye_position_xmv = XMLoadFloat3(&eye_position);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&eye_direction);
    eye_direction_xmv = XMVector3Normalize(eye_direction_xmv);

    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&eye_position, eye_position_xmv);
    XMStoreFloat3(&eye_direction, eye_direction_xmv);
}

void Camera::translate(uint32_t keycode, float delta_time)
{
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR eye_position_xmv = XMLoadFloat3(&eye_position);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&eye_direction);
    
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, world_up));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, right));

    const float ms_v = 100.f * delta_time;
    XMVECTOR ms = XMVectorSet(ms_v, ms_v, ms_v, ms_v);
    
    if(keycode & 0x08)  // 'W'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if(keycode & 0x04)  // 'S'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if(keycode & 0x01)  // 'A'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(right, ms));
    if(keycode & 0x02)  // 'D'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(right, ms));
    
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&eye_position, eye_position_xmv);
    XMStoreFloat3(&eye_direction, eye_direction_xmv);
}

void Camera::translate(DirectX::XMVECTOR pos_offset)
{
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&eye_direction);
    XMVECTOR eye_position_xmv = XMLoadFloat3(&eye_position);
    eye_position_xmv = XMVectorAdd(eye_position_xmv, pos_offset);

    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&eye_position, eye_position_xmv);
}


}