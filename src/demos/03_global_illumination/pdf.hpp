#pragma once
#include "../../simple_math.hpp"

namespace moonlight
{

class PDF
{
public:
    virtual ~PDF() {}

    virtual float value(const Vector3<float>& direction) const = 0;
    virtual Vector3<float> generate() const = 0;
};

}