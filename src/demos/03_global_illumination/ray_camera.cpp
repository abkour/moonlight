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
    , pitch(0.f)
    , yaw(0.f)
    , half_fov_in_radians(0.f)
{}

void RayCamera::initializeVariables(
    const Vector3f& pos, 
    const Vector3f& dir,
    const float fov_in_degrees, 
    const unsigned nSamples) 
{
    eyedir = dir;
    eyepos = pos;
    half_fov_in_radians = radians(fov_in_degrees / 2.f);
    const Vector3f T(dir - pos);
    const Vector3f up(0.f, 1.f, 0.f);
    const Vector3f right_norm(normalize(cross(up, T)));
    const Vector3f t_norm(normalize(T));
    const Vector3f up_norm(cross(t_norm, right_norm));

    const float aspect_ratio = ((float)resolution.x - 1) / ((float)resolution.y - 1);
    const float gx = std::tan(half_fov_in_radians);
    const float gy = gx * (((float)resolution.y - 1) / ((float)resolution.x - 1));

    shiftx = right_norm * ((2 * gx) / (((float)resolution.x - 1)));
    shifty = up_norm * ((2 * gy) / (((float)resolution.y - 1)));
    topLeftPixel = t_norm - (right_norm * gx) - (up_norm * gy);
}

bool RayCamera::camera_variables_need_updating()
{
    if (need_update)
    {
        need_update = false;
        return true;
    }
    return false;
}

void RayCamera::rotate(float dx, float dy)
{
    if (dx != 0.f || dy != 0.f)
        need_update = true;

    const float scale = 0.15f;
    yaw += dx * scale;
    pitch += dy * scale;

    if (pitch > 89.f)
        pitch = 89.f;
    else if (pitch < -89.f)
        pitch = -89.f;

    eyedir.x = std::cos(radians(yaw)) * std::cos(radians(pitch));
    eyedir.y = std::sin(radians(pitch));
    eyedir.z = std::sin(radians(yaw)) * std::cos(radians(pitch));
    eyedir = normalize(eyedir);
}

void RayCamera::translate(KeyState kd, const float dt)
{
    Vector3<float> world_up(0.f, 1.f, 0.f);

    Vector3<float> right = normalize(cross(eyedir, world_up));
    Vector3<float> up    = normalize(cross(eyedir, right));

    const float ms_v = movement_speed * dt;
    
    if (kd['A'] || kd['D'] || kd['S'] || kd['W'])
        need_update = true;

    if (kd['W'])  // 'W'
        eyepos = eyepos + eyedir * ms_v;
    if (kd['S'])  // 'S'
        eyepos = eyepos - eyedir * ms_v;
    if (kd['A'])  // 'A'
        eyepos = eyepos + right * ms_v;
    if (kd['D'])  // 'D'
        eyepos = eyepos - right * ms_v;
}

void RayCamera::reinitialize_camera_variables()
{
    const Vector3f T(eyedir);
    const Vector3f up(0.f, 1.f, 0.f);
    const Vector3f right_norm(normalize(cross(up, T)));
    const Vector3f t_norm(normalize(T));
    const Vector3f up_norm(cross(t_norm, right_norm));

    const float aspect_ratio = ((float)resolution.x - 1) / ((float)resolution.y - 1);
    const float gx = std::tan(half_fov_in_radians);
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

void RayCamera::set_movement_speed(const float movement_speed)
{
    this->movement_speed = movement_speed;
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