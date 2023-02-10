#pragma once
#include "simple_math.hpp"
#include <Windows.h>
#include <DirectXMath.h>

namespace moonlight
{

struct Camera
{
	Camera(DirectX::XMFLOAT3 eye_position, DirectX::XMFLOAT3 eye_direction);

	void rotate(float dx, float dy);
	void translate(WPARAM key, float delta_time);
	void translate(DirectX::XMVECTOR pos_offset);

	DirectX::XMFLOAT3 get_direction() const
	{
		return eye_direction;
	}

	DirectX::XMFLOAT3 get_position() const
	{
		return eye_position;
	}

	DirectX::XMMATRIX view;

private:

	float pitch, yaw;
	DirectX::XMFLOAT3 eye_position, eye_direction;
};

}