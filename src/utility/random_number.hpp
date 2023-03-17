#pragma once
#include <random>

#include "../simple_math.hpp"

namespace moonlight
{

float random_in_range(float r0, float r1);
float random_normalized_float();

Vector3<float> random_unit_vector();

}