#pragma once
#include "../../math/ray.hpp"
#include <DirectXMath.h>

namespace moonlight
{

struct RayCamera {

    RayCamera() = default;

    // Initialize the virtual camera
    RayCamera(const Vector2<uint16_t> resolution);

    // Compute the top-left pixel location of the virtual camera in addition to 
    // the shifting vectors used to shift the top-left pixel.
    // Needs to be called every time either of the three arguments change.
    void initializeVariables(
        const Vector3<float>& pos, 
        const Vector3<float>& dir, 
        const float fov_in_degrees, 
        const unsigned nSamples
    );

    // Compute a ray originating from the eye position towards the pixelLocation
    // in world space.
    Ray getRay(const Vector2<uint16_t>& pixelLocation);

    void setResolution(Vector2<uint16_t> newResolution);
    unsigned resx() const;
    unsigned resy() const;

    inline Vector3<float> get_position() const
    {
        return eyepos;
    }

protected:

    Vector2<uint16_t> resolution;
    Vector3<float> eyepos;
    Vector3<float> shiftx, shifty, topLeftPixel;
};

}