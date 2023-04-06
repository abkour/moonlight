#include "shape.hpp"
#include "../coordinate_system.hpp"
#include "../samplers.hpp"
#include "../../../project_defines.hpp"

namespace moonlight
{

class Circle : public Shape
{
public:

    Circle(Vector3<float> center, Vector3<float> normal, float radius)
        : center(center), normal(normal), radius(radius)
        , cs(normal)
    {
        m_area = radius * radius * ML_PI;
    }

    float area() const override
    {
        return m_area;
    }

    IntersectionParams intersect(const Ray& ray) override
    {
        IntersectionParams its;

        float t = (dot(normal, center) - dot(normal, ray.o)) / dot(normal, ray.d);

        Vector3<float> p = ray.o + t * ray.d;
        Vector3<float> q = center - p;
        if (dot(normal, q) == 0)
        {
            if (length(q) <= radius)
            {
                its.t = t;
            }
        }

        return its;
    }

    Vector3<float> sample() override
    {
        Vector2<float> sample;
        sample_concentrid_disk(sample);  
        sample *= radius; // is multiplying by the radius still ensuring uniformity?
        
        Vector3<float> sample_3d(sample.x, sample.y, 0.f);

        return cs.to_local(sample_3d) + center;
    }

private:

    float m_area;
    float radius;
    Vector3<float> center;
    Vector3<float> normal;

    CoordinateSystem cs;
};

}