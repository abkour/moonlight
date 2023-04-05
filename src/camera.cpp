#include "camera.hpp"

using namespace DirectX;;

namespace moonlight
{

Camera::Camera(
    DirectX::XMFLOAT3 eye_position, 
    DirectX::XMFLOAT3 eye_direction, 
    const float movement_speed)
{
    m_pitch = 0.f;
    m_yaw = 0.f;
    m_movement_speed = movement_speed;
    m_eye_direction = eye_direction;
    m_eye_position = eye_position;

    XMVECTOR eye_position_xmv = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&eye_position));
    XMVECTOR eye_direction_xmv = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(&eye_direction));
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);
}

void Camera::rotate(float dx, float dy)
{
    const float scale = 0.15f;
    m_yaw += dx * scale;
    m_pitch += dy * scale;

    if (m_pitch > 89.f)
    {
        m_pitch = 89.f;
    }
    else if (m_pitch < -89.f)
    {
        m_pitch = -89.f;
    }

    m_eye_direction.x = std::cos(radians(m_yaw)) * std::cos(radians(m_pitch));
    m_eye_direction.y = std::sin(radians(m_pitch));
    m_eye_direction.z = std::sin(radians(m_yaw)) * std::cos(radians(m_pitch));

    XMVECTOR eye_position_xmv = XMLoadFloat3(&m_eye_position);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&m_eye_direction);
    eye_direction_xmv = XMVector3Normalize(eye_direction_xmv);

    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&m_eye_position, eye_position_xmv);
    XMStoreFloat3(&m_eye_direction, eye_direction_xmv);
}

void Camera::translate(KeyState keys, float delta_time)
{
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR eye_position_xmv = XMLoadFloat3(&m_eye_position);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&m_eye_direction);
    
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, world_up));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, right));

    const float ms_v = m_movement_speed * delta_time;
    XMVECTOR ms = XMVectorSet(ms_v, ms_v, ms_v, ms_v);
    
    if(keys['W'])  // 'W'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if(keys['S'])  // 'S'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if(keys['A'])  // 'A'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(right, ms));
    if(keys['D'])  // 'D'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(right, ms));
    
    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&m_eye_position, eye_position_xmv);
    XMStoreFloat3(&m_eye_direction, eye_direction_xmv);
}

void Camera::translate(DirectX::XMVECTOR pos_offset)
{
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&m_eye_direction);
    XMVECTOR eye_position_xmv = XMLoadFloat3(&m_eye_position);
    eye_position_xmv = XMVectorAdd(eye_position_xmv, pos_offset);

    view = XMMatrixLookAtLH(eye_position_xmv, eye_position_xmv + eye_direction_xmv, world_up);

    XMStoreFloat3(&m_eye_position, eye_position_xmv);
}

void Camera::translate_around_center(KeyState keys, DirectX::XMVECTOR center, float delta_time)
{
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR eye_position_xmv = XMLoadFloat3(&m_eye_position);
    XMVECTOR eye_direction_xmv = XMLoadFloat3(&m_eye_direction);

    XMVECTOR right = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, world_up));
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, right));

    const float ms_v = m_movement_speed * delta_time;
    XMVECTOR ms = XMVectorSet(ms_v, ms_v, ms_v, ms_v);

    if (keys['W'])  // 'W'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if (keys['S'])  // 'S'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
    if (keys['A'])  // 'A'
        eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(right, ms));
    if (keys['D'])  // 'D'
        eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(right, ms));

    view = XMMatrixLookAtLH(eye_position_xmv, center, world_up);

    XMStoreFloat3(&m_eye_position, eye_position_xmv);
    XMStoreFloat3(&m_eye_direction, eye_direction_xmv);
}



}