#pragma once
#include "light.hpp"
#include "pdf.hpp"

namespace moonlight
{

class LightPDF : public PDF
{
public:
    
    LightPDF(ILight* light, const Vector3<float>& origin)
        : m_light(light)
        , m_origin(origin)
    {}

    float value(const Vector3<float>& direction) const override
    {
        return m_light->pdf(m_origin, direction);
    }
    
    Vector3<float> generate() const override
    {
        return m_light->sample(m_origin);
    }
    
private:

    Vector3<float> m_origin;
    ILight* m_light;
};

}