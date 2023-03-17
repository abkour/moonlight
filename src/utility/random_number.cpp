#include "random_number.hpp"

namespace moonlight
{

float random_in_range(float r0, float r1)
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution<float> dist(r0, r1);
    return dist(mt);
}

float random_normalized_float()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(mt);
}

Vector3<float> random_unit_vector()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution<float> dist(-1.f, 1.f);

    while (true)
    {
        Vector3<float> p(dist(mt), dist(mt), dist(mt));
        if (dot(p, p) >= 1) continue;
        return p;
    }
}


}