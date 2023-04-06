#include "light.hpp"
#include "samplers.hpp"
#include "../../utility/random_number.hpp"
#include "../../collision/ray.hpp"
#include "shapes/rectangle.hpp"

namespace moonlight
{

class AreaLight : public ILight
{
public:

    AreaLight(Vector3<float> albedo, Shape* shape)
        : m_shape(shape)
    {
        m_albedo = albedo;
    }

    AreaLight(Vector3<float> albedo, std::shared_ptr<Shape>& shape)
        : m_shape(shape)
    {
        m_albedo = albedo;
    }

    void sample_light(const IntersectionParams& its, float* pdf, bool* visibile) override
    {
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

        float distance_squared = dot(dir, dir);
        float theta = fabs(dot(dir, its.normal) / length(dir));

        return distance_squared / (theta * m_shape->area());
    }

    void sample(
        Ray& r_out, 
        const Ray& r_in, 
        float& pdf, 
        const IntersectionParams& its) override
    {
        Vector3<float> p = m_shape->sample();
        Vector3<float> dir = p - its.point;
        Vector3<float> n_dir = normalize(dir);
        
        r_out = Ray(its.point + n_dir * 1e-4, n_dir);
        IntersectionParams new_its = this->intersect(r_out);

        // Compute the pdf
        float distance_squared = dot(dir, dir);
        float theta = fabs(dot(dir, new_its.normal) / length(dir));

        pdf = distance_squared / (theta * m_shape->area());
    }

    Vector3<float> sample(const Vector3<float>& origin)
    {
        return m_shape->sample() - origin;
    }
    
    IntersectionParams intersect(const Ray& ray) override
    {
        return m_shape->intersect(ray);
    }

private:

    std::shared_ptr<Shape> m_shape;
};

}