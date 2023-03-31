#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

struct CoordinateSystem
{
    CoordinateSystem(const Vector3<float>& normal);

    Vector3<float> to_local(const Vector3<float>& p) const;
    
    Vector3<float> n, nb, nt;
};

}