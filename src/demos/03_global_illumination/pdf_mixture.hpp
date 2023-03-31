#pragma once
#include "pdf.hpp"
#include "../../utility/random_number.hpp"

namespace moonlight
{

struct MixturePDF : public PDF
{
    MixturePDF(PDF* p0, PDF* p1)
        : p0(p0)
        , p1(p1)
    {
    }

    float value(const Vector3<float>& direction) const override
    {
        return 0.5 * p0->value(direction) + 0.5 * p1->value(direction);
    }

    Vector3<float> generate() const override
    {
        if (random_in_range(0.f, 1.f) < 0.5f)
        {
            return p0->generate();
        } else
        {
            return p1->generate();
        }
    }

    PDF* p0;
    PDF* p1;
};

}