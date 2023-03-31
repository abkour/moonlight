#include "light.hpp"
#include "samplers.hpp"
#include "../../utility/random_number.hpp"
#include "../../collision/ray.hpp"

namespace moonlight
{

class RectangularLight : public ILight
{
public:

    RectangularLight(Vector3<float> albedo,
        Vector3<float> v0,
        Vector3<float> v1,
        Vector3<float> v2,
        Vector3<float> v3)
        : v0(v0), v1(v1), v2(v2), v3(v3)
        , ILight(albedo)
    {
        Vector3<float> e0 = v1 - v0;
        Vector3<float> e1 = v2 - v0;
        area = length(cross(e0, e1));
    }

    virtual float pdf(const Vector3<float>& origin, const Vector3<float>& dir)
    {
        Vector3<float> normalized_dir = normalize(dir);
        Ray ray(origin + normalized_dir * 1e-3, normalized_dir);
        IntersectionParams its = this->intersect(ray);

        if (!its.is_intersection())
        {
            return 0.f;
        }

        if (its.front_face && dot(its.normal, normalized_dir) < 0.f)
        {
            return 0.f;
        }

        float distance_squared = dot(dir, dir);
        float cosine = fabs(dot(dir, its.normal) / length(dir));

        return distance_squared / (cosine * area);
    }

    Vector3<float> sample(const Vector3<float>& origin)
    {
        Vector2<float> r(random_in_range(0.f, 1.f), random_in_range(0.f, 1.f));
        r = sample_triangle(r);
        float t = random_in_range(0.f, 1.f);
        Vector3<float> p;
        if (t < 0.5f)
        {
            p = r[0] * v0 + r[1] * v1 + (1 - r[0] - r[1]) * v3;
        } 
        else
        {
            p = r[0] * v0 + r[1] * v2 + (1 - r[0] - r[1]) * v3;
        }

        return p - origin;
    }
    
    IntersectionParams intersect(const Ray& ray) override
    {
        IntersectionParams intersect =
            ray_hit_triangle(ray, (float*)&v0, (float*)&v1, (float*)&v2, 3);

        if (intersect.is_intersection())
        {
            return intersect;
        }

        return ray_hit_triangle(ray, (float*)&v0, (float*)&v2, (float*)&v3, 3);
    }

protected:

    float area;
    Vector3<float> v0, v1, v2, v3;
};

}