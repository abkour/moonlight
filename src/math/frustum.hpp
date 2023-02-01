#pragma once
#include "aabb.hpp"
#include "plane.hpp"
#include <cmath>
#include <fstream>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

namespace moonlight
{

struct Frustum
{
	// The normals for these planes point away from the frustum
	Plane near, far;
	Plane left, right;
	Plane bottom, top;
};

// Again, unoptimized. This is proof-of-work, don't copy.
static Frustum construct_frustum(
	Vector3 eye_position,
	Vector3 eye_direction, // requires normalized
	float vfov,
	float aspect_ratio,
	float near_distance,
	float far_distance)
{
	float near_hh = tan(vfov / 2.f) * near_distance;
	float near_hw = near_hh * aspect_ratio;
	// Law of similar triangles
	float far_hh = near_hh * (far_distance - near_distance); 
	float far_hw = near_hw * (far_distance - near_distance);

	Vector3 world_up(0.f, 1.f, 0.f);
	Vector3 right = cross(world_up, eye_direction);
	right = normalize(right);
	Vector3 up = cross(eye_direction, right);
	up = normalize(up);

	Frustum frustum;
	frustum.near.point = eye_position;
	frustum.left.point = eye_position;
	frustum.top.point = eye_position;
	frustum.bottom.point = eye_position;
	frustum.right.point = eye_position;

	/*
	*	This is the near plane as seen from the perspective of the eye.
	*	We compute the points mxx and cxx to construct the plane normals.
	*	The normals for the near/far plane are given by the eye_direction
	* 
		c00----m10----ooo
		|               |
		|               |
		m00     c     m11
		|               |
		|               |
		ooo----m01----c11
	*/
	Vector3 c = eye_position + eye_direction * near_distance;
	Vector3 m00 = c + invert(right) * near_hw;
	Vector3 m11 = c + right * near_hw;
	Vector3 m10 = c + up * near_hh;
	Vector3 m01 = c + invert(up) * near_hh;
	Vector3 c00 = m00 + up * near_hh;
	Vector3 c11 = m01 + right * near_hw;

	frustum.far.normal = eye_direction;
	frustum.near.normal = invert(eye_direction);
	frustum.left.normal = normalize(cross(
		m00 - eye_position, c00 - eye_position
	));
	frustum.right.normal = normalize(cross(
		m11 - eye_position, c11 - eye_position
	));
	frustum.top.normal = normalize(cross(
		c00 - eye_position, m10 - eye_position
	));
	frustum.bottom.normal = normalize(cross(
		c11 - eye_position, m01 - eye_position
	));

	// Compute the center of the far plane 
	frustum.far.point = eye_position + (normalize(c - eye_position) * far_distance);

	return frustum;
}

// I wouldn't use this in actual code. This is only for proof-of-concept
bool frustum_contains_point(const Frustum& frustum, const Vector3& point)
{
	if (point_plane_intersection(frustum.near, point) == 1)
	{
		return false;
	}
	if (point_plane_intersection(frustum.left, point) == 1)
	{
		return false;
	}
	if (point_plane_intersection(frustum.right, point) == 1)
	{
		return false;
	}
	if (point_plane_intersection(frustum.bottom, point) == 1)
	{
		return false;
	}
	if (point_plane_intersection(frustum.top, point) == 1)
	{
		return false;
	}
	if (point_plane_intersection(frustum.far, point) == 1)
	{
		return false;
	}

	return true;
}

}