#pragma once
#include <random>

#include "../simple_math.hpp"

namespace moonlight
{

struct RandomFloatEngine
{
    void seed(unsigned int value)
    {
        mt.seed(value);
    }

    float get_normalized()
    {
        return dist(mt);
    }

    float get_unit_length()
    {
        return dist_unit(mt);
    }

    Vector3<float> get_unit_vector()
    {
        while (true)
        {
            Vector3<float> p(dist_unit(mt), dist_unit(mt), dist_unit(mt));
            if (dot(p, p) >= 1) continue;
            return p;
        }
    }

    std::mt19937 mt;
    std::uniform_real_distribution<float> dist;
    std::uniform_real_distribution<float> dist_unit;
};

float random_in_range(float r0, float r1);
float random_normalized_float();

Vector3<float> random_unit_vector();

}