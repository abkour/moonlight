#include "ray_camera.hpp"

using namespace DirectX;

namespace moonlight {

Camera::Camera(const XMFLOAT2 resolution)
    : resolution(resolution)
    , eyepos(0)
    , shiftx(0)
    , shifty(0)
    , topLeftPixel(0)
{}

void Camera::initializeVariables(const Vector3& pos, const Vector3& dir, 
    const float fov, const unsigned nSamples) 
{
    eyepos = pos;
    const Vector3 T(dir - pos);
    const Vector3 up(0.f, 1.f, 0.f);
    const Vector3 right_norm(normalize(cross(up, T)));
    const Vector3 t_norm(normalize(T));
    const Vector3 up_norm(cross(t_norm, right_norm));
    const float aspect_ratio = ((float)resolution.x - 1) / ((float)resolution.y - 1);
    const float gx = std::tan(radians(fov / 2.f));
    const float gy = gx * (((float)resolution.y - 1) / ((float)resolution.x - 1));

    shiftx = right_norm * ((2 * gx) / (((float)resolution.x - 1)));
    shifty = up_norm * ((2 * gy) / (((float)resolution.y - 1)));
    topLeftPixel = t_norm - (right_norm * gx) - (up_norm * gy);
}

Ray Camera::getRay(const XMFLOAT2& pixelLocation) {
    Vector3 origin = eyepos;
    Vector3 direction = topLeftPixel + (shiftx * (pixelLocation.x - 1.f)) + (shifty * (pixelLocation.y - 1.f));
    return Ray(origin, normalize(direction));
}

void Camera::setResolution(XMFLOAT2 newResolution) {
    resolution = newResolution;
}

unsigned Camera::resx() const {
    return resolution.x;
}

unsigned Camera::resy() const {
    return resolution.y;
}

}