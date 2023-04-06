#include "shape.hpp"
#include "../../../utility/random_number.hpp"
#include "../samplers.hpp"

namespace moonlight
{

class Rectangle : public Shape
{
public:
    
    Rectangle(Vector3<float> v0,
              Vector3<float> v1,
              Vector3<float> v2,
              Vector3<float> v3)
        : v0(v0), v1(v1), v2(v2), v3(v3)
    {
        Vector3<float> e0 = v1 - v0;
        Vector3<float> e1 = v2 - v0;
        Vector3<float> cp = cross(e0, e1);
        m_area = length(cp);
        normal = normalize(cp);
    }

    float area() const override
    {
        return m_area;
    }

    IntersectionParams intersect(const Ray& ray) override
    {
        IntersectionParams intersect =
            ray_hit_triangle(ray, v0, v1, v2);

        if (intersect.is_intersection())
        {
            return intersect;
        }

        return ray_hit_triangle(ray, v0, v2, v3);
    }

    Vector3<float> sample() override
    {
        Vector2<float> r(random_in_range(0.f, 1.f), random_in_range(0.f, 1.f));
        float t = r.x;
        r = sample_triangle(r);
        if (t < 0.5f)
        {
            return r[0] * v0 + r[1] * v1 + (1 - r[0] - r[1]) * v3;
        } 
        
        return r[0] * v0 + r[1] * v2 + (1 - r[0] - r[1]) * v3;
    }

private:

    float m_area;
    Vector3<float> v0, v1, v2, v3;
    Vector3<float> normal;
};

}