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

void Camera::translate(WPARAM key, float delta_time)
{
	XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
	XMVECTOR eye_position_xmv = XMLoadFloat3(&eye_position);
	XMVECTOR eye_direction_xmv = XMLoadFloat3(&eye_direction);
	
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, world_up));
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(eye_direction_xmv, right));

	const float ms_v = 2.f;
	XMVECTOR ms = XMVectorSet(ms_v, ms_v, ms_v, ms_v);
	
	switch (key)
	{
	case 'W':
		eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
		break;
	case 'S':
		eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(eye_direction_xmv, ms));
		break;
	case 'A':
		eye_position_xmv = XMVectorAdd(eye_position_xmv, XMVectorMultiply(right, ms));
		break;
	case 'D':
		eye_position_xmv = XMVectorSubtract(eye_position_xmv, XMVectorMultiply(right, ms));
		break;
	default:
		break;
	}

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