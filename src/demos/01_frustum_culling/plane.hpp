#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

struct Plane
{
	Vector3 normal;
	Vector3 point;
};

int point_plane_intersection(Plane& plane, const Vector3& p)
{
	Vector3 X = normalize(p - plane.point);
	float result = dot(plane.normal, X);
	
	if (result > 0.f)
	{
		return 1;
	}
	else if (result < 0.f)
	{
		return -1;
	}
	
	return 0;
}

}