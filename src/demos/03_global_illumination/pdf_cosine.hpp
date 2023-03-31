#include "pdf.hpp"
#include "coordinate_system.hpp"
#include "../../utility/random_number.hpp"
#include "../../project_defines.hpp"

namespace moonlight
{

class CosinePDF : public PDF
{
public:

    CosinePDF(const Vector3<float>& normal)
        : cs(normal)
    {}

    float value(const Vector3<float>& direction) const override
    {
        auto cosine = dot(normalize(direction), cs.n);
        return cosine <= 0 ? 0 : cosine / ML_PI;
    }

    Vector3<float> generate() const override
    {
        return cs.to_local(random_cosine_direction());
    }

    CoordinateSystem cs;
};

}