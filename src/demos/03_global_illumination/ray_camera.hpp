#pragma once
#include "../../collision/ray.hpp"
#include "../../core/key_state.hpp"
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

    bool camera_variables_need_updating();

    void rotate(float dx, float dy);
    void translate(KeyState kd, const float dt);
    
    void reinitialize_camera_variables();

    // Compute a ray originating from the eye position towards the pixelLocation
    // in world space.
    Ray getRay(const Vector2<uint16_t>& pixelLocation);

    void set_movement_speed(const float movement_speed);
    void setResolution(Vector2<uint16_t> newResolution);
    unsigned resx() const;
    unsigned resy() const;

    inline Vector3<float> get_position() const
    {
        return eyepos;
    }

protected:

    Vector2<uint16_t> resolution;
    Vector3<float> eyepos, eyedir;
    Vector3<float> shiftx, shifty, topLeftPixel;

    float half_fov_in_radians;
    float yaw; 
    float pitch;
    float movement_speed = 1.f;

    bool need_update = false;
};

}