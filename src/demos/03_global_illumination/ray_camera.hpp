#pragma once
#include "ray.hpp"
#include <DirectXMath.h>

namespace moonlight
{

struct Camera {

    Camera() = default;

    // Initialize the virtual camera
    Camera(const DirectX::XMFLOAT2 resolution);

    // Compute the top-left pixel location of the virtual camera in addition to 
    // the shifting vectors used to shift the top-left pixel.
    // Needs to be called every time either of the three arguments change.
    void initializeVariables(const Vector3& pos, const Vector3& dir, const float fov, const unsigned nSamples);

    // Compute a ray originating from the eye position towards the pixelLocation
    // in world space.
    Ray getRay(const DirectX::XMFLOAT2& pixelLocation);

    void setResolution(DirectX::XMFLOAT2 newResolution);
    unsigned resx() const;
    unsigned resy() const;

protected:

    DirectX::XMFLOAT2 resolution;
    Vector3 eyepos;
    Vector3 shiftx, shifty, topLeftPixel;
};

}