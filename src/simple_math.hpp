/*
    Yet another math library?

    Well, the reason for this is that I don't want to use the DirectXMath functionality.
    It is too cumbersome to use in the context of demo applications.
    I don't want to use a 3rd party library either, because I want to keep the depedencies
    at a minimum.

    We follow the YAGNI principle here, and only impleeent functionality that is
    required for demos.

    I will use the DirectXMath functionality for certain computations (projection matrices, lookat)
    for now.
*/
#pragma once
#include <cmath>
#include <numbers>

namespace moonlight
{

struct Vector3
{
    Vector3()
        : x(0.f), y(0.f), z(0.f)
    {}

    Vector3(float val)
        : x(val), y(val), z(val)
    {}

    Vector3(float xx, float yy,  float zz)
        : x(xx), y(yy), z(zz)
    {}

    Vector3(const Vector3& other)
        : x(other.x), y(other.y), z(other.z)
    {}

    Vector3(Vector3&& other)
        : x(other.x), y(other.y), z(other.z)
    {}

    Vector3& operator=(const Vector3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    Vector3& operator=(Vector3&& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    bool operator==(const Vector3& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3& other) const
    {
        return !(this->operator==(other));
    }

    float operator[](const uint8_t i) const
    {
        return v[i];
    }

    float& operator[](const uint8_t i)
    {
        return v[i];
    }

    Vector3 operator+(const Vector3& v0) const
    {
        return { x + v0.x, y + v0.y, z + v0.z };
    }

    Vector3 operator-(const Vector3& v0) const
    {
        return { x - v0.x, y - v0.y, z - v0.z };
    }

    Vector3 operator*(const Vector3& v0) const
    {
        return { x * v0.x, y * v0.y, z * v0.z };
    }

    Vector3 operator*(const float val) const
    {
        return { x * val, y * val, z * val };
    }

    Vector3 operator/(const float val) const
    {
        return this->operator*(1.f / val);
    }

    Vector3& operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3& operator*=(const Vector3& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    Vector3& operator*=(const float val)
    {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }

    Vector3& operator/=(const float val)
    {
        x /= val;
        y /= val;
        z /= val;
        return *this;
    }

    union
    {
        struct
        {
            float x, y, z;
        };
        float v[3];
    };
};

inline float dot(const Vector3& v0, const Vector3& v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

inline float length(const Vector3& v0)
{
    return std::sqrt(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z);
}

inline Vector3 invert(const Vector3& v0)
{
    return { -v0.x, -v0.y, -v0.z };
}

inline Vector3 normalize(const Vector3& v0)
{
    return v0 / length(v0);
}

inline Vector3 cross(const Vector3& v0, const Vector3& v1)
{
    return {
        v0.y * v1.z - v0.z * v1.y,
        v0.z * v1.x - v0.x * v1.z,
        v0.x * v1.y - v0.y * v1.x
    };
}

inline bool cwise_greater(const Vector3& v0, const Vector3& v1)
{
    return v0.x > v1.x && v0.y > v1.y && v0.z > v1.z;
}

inline bool cwise_greater_or_equal(const Vector3& v0, const Vector3& v1)
{
    return v0.x >= v1.x && v0.y >= v1.y && v0.z >= v1.z;
}

inline bool cwise_less(const Vector3& v0, const Vector3& v1)
{
    return v0.x < v1.x && v0.y < v1.y && v0.z < v1.z;
}

inline bool cwise_less_or_equal(const Vector3& v0, const Vector3& v1)
{
    return v0.x <= v1.x&& v0.y <= v1.y&& v0.z <= v1.z;
}

inline float radians(float x_in_degrees)
{
    return (x_in_degrees * std::numbers::pi_v<float>) / 180.f;
}

}