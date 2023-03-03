#include "ray_camera.hpp"

using namespace DirectX;

namespace moonlight {

using Vector2s = Vector2<uint16_t>;
using Vector3f = Vector3<float>;

RayCamera::RayCamera(const Vector2s resolution)
    : resolution(resolution)
    , eyepos(0)
    , shiftx(0)
    , shifty(0)
    , topLeftPixel(0)
{}

void RayCamera::initializeVariables(
    const Vector3f& pos, 
    const Vector3f& dir,
    const float fov_in_degrees, 
    const unsigned nSamples) 
{
    eyepos = pos;
    const Vector3f T(dir - pos);
    const Vector3f up(0.f, 1.f, 0.f);
    const Vector3f right_norm(normalize(cross(up, T)));
    const Vector3f t_norm(normalize(T));
    const Vector3f up_norm(cross(t_norm, right_norm));

    const float aspect_ratio = ((float)resolution.x - 1) / ((float)resolution.y - 1);
    const float gx = std::tan(radians(fov_in_degrees / 2.f));
    const float gy = gx * (((float)resolution.y - 1) / ((float)resolution.x - 1));

    shiftx = right_norm * ((2 * gx) / (((float)resolution.x - 1)));
    shifty = up_norm * ((2 * gy) / (((float)resolution.y - 1)));
    topLeftPixel = t_norm - (right_norm * gx) - (up_norm * gy);
}

Ray RayCamera::getRay(const Vector2s& pixelLocation) 
{
    Vector3f direction = 
        topLeftPixel + 
        (shiftx * ((float)pixelLocation.x - 1.f)) + 
        (shifty * ((float)pixelLocation.y - 1.f));

    return Ray(eyepos, normalize(direction));
}

void RayCamera::setResolution(Vector2s newResolution) 
{
    resolution = newResolution;
}

unsigned RayCamera::resx() const 
{
    return resolution.x;
}

unsigned RayCamera::resy() const 
{
    return resolution.y;
}

}