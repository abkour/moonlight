#pragma once
#include "../../../simple_math.hpp"
#include "../../../collision/ray.hpp"

namespace moonlight
{

class Shape
{
public:

    virtual float area() const = 0;
    virtual IntersectionParams intersect(const Ray& ray) = 0;
    virtual Vector3<float> sample() = 0;
};

}