#include "random_number.hpp"

namespace moonlight
{

Vector3<float> random_cosine_direction()
{
    auto r1 = random_in_range(0.f, 1.f);
    auto r2 = random_in_range(0.f, 1.f);
    auto z = sqrt(1 - r2);

    auto phi = 2 * ML_PI * r1;
    auto x = cos(phi) * sqrt(r2);
    auto y = sin(phi) * sqrt(r2);

    return Vector3<float>(x, y, z);
}

float random_in_range(float r0, float r1)
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(r0, r1);
    return dist(mt);
}

float random_normalized_float()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    return dist(mt);
}

int random_in_range_int(int r0, int r1)
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(r0, r1);
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