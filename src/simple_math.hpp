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
#include <ostream>

namespace moonlight
{

template<typename T>
struct Vector2
{
    Vector2()
        : x(0.f), y(0.f)
    {}

    Vector2(T val)
        : x(val), y(val)
    {}

    Vector2(T xx, T yy)
        : x(xx), y(yy)
    {}

    Vector2(const Vector2<T>& other)
        : x(other.x), y(other.y)
    {}

    Vector2(Vector2<T>&& other)
        : x(other.x), y(other.y)
    {}

    Vector2<T>& operator=(const Vector2<T>& other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }

    Vector2<T>& operator=(Vector2<T>&& other)
    {
        x = other.x;
        y = other.y;
        return *this;
    }

    bool operator==(const Vector2<T>& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2<T>& other) const
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

    Vector2<T> operator+(const Vector2<T>& v0) const
    {
        return { x + v0.x, y + v0.y };
    }

    Vector2<T> operator-(const Vector2<T>& v0) const
    {
        return { x - v0.x, y - v0.y };
    }

    Vector2<T> operator*(const Vector2<T>& v0) const
    {
        return { x * v0.x, y * v0.y };
    }

    Vector2<T> operator*(const T val) const
    {
        return { x * val, y * val };
    }

    Vector2<T> operator/(const T val) const
    {
        return this->operator*(1.f / val);
    }

    Vector2<T>& operator+=(const Vector2<T>& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2<T>& operator-=(const Vector2<T>& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2<T>& operator*=(const Vector2<T>& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    Vector2<T>& operator*=(const T val)
    {
        x *= val;
        y *= val;
        return *this;
    }

    Vector2<T>& operator/=(const T val)
    {
        x /= val;
        y /= val;
        return *this;
    }

    union
    {
        struct
        {
            T x, y;
        };
        T v[2];
    };
};

template<typename T>
Vector2<T> operator*(const T t, const Vector2<T>& v)
{
    return { v.x * t, v.y * t };
}

template<typename T>
T dot(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return v0.x * v1.x + v0.y * v1.y;
}

template<typename T>
T length(const Vector2<T>& v0)
{
    return std::sqrt(v0.x * v0.x + v0.y * v0.y);
}

template<typename T>
Vector2<T> invert(const Vector2<T>& v0)
{
    return { -v0.x, -v0.y };
}

template<typename T>
Vector2<T> normalize(const Vector2<T>& v0)
{
    return v0 / length(v0);
}

template<typename T>
bool cwise_greater(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return v0.x > v1.x && v0.y > v1.y;
}

template<typename T>
bool cwise_greater_or_equal(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return v0.x >= v1.x && v0.y >= v1.y;
}

template<typename T>
bool cwise_less(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return v0.x < v1.x&& v0.y < v1.y;
}

template<typename T>
bool cwise_less_or_equal(const Vector2<T>& v0, const Vector2<T>& v1)
{
    return v0.x <= v1.x && v0.y <= v1.y;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vector2<T>& vec)
{
    return os << vec.x << ", " << vec.y;
}

template<typename T>
struct Vector3
{
    Vector3()
        : x(0.f), y(0.f), z(0.f)
    {}

    Vector3(T val)
        : x(val), y(val), z(val)
    {}

    Vector3(T xx, T yy,  T zz)
        : x(xx), y(yy), z(zz)
    {}

    Vector3(const Vector3<T>& other)
        : x(other.x), y(other.y), z(other.z)
    {}

    Vector3(Vector3<T>&& other)
        : x(other.x), y(other.y), z(other.z)
    {}

    Vector3<T>& operator=(const Vector3<T>& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    Vector3<T>& operator=(Vector3<T>&& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    bool operator==(const Vector3<T>& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Vector3<T>& other) const
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

    Vector3<T> operator+(const Vector3<T>& v0) const
    {
        return { x + v0.x, y + v0.y, z + v0.z };
    }

    Vector3<T> operator-(const Vector3<T>& v0) const
    {
        return { x - v0.x, y - v0.y, z - v0.z };
    }

    Vector3<T> operator*(const Vector3<T>& v0) const
    {
        return { x * v0.x, y * v0.y, z * v0.z };
    }

    Vector3<T> operator*(const T val) const
    {
        return { x * val, y * val, z * val };
    }

    Vector3<T> operator/(const T val) const
    {
        return this->operator*(1.f / val);
    }

    Vector3<T>& operator+=(const Vector3<T>& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3<T>& operator-=(const Vector3<T>& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3<T>& operator*=(const Vector3<T>& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    Vector3<T>& operator*=(const T val)
    {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }

    Vector3<T>& operator/=(const T val)
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
            T x, y, z;
        };
        T v[3];
    };
};

template<typename T>
Vector3<T> operator*(const T t, const Vector3<T>& v)
{
    return { v.x * t, v.y * t, v.z * t };
}

template<typename T>
T dot(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

template<typename T>
T length(const Vector3<T>& v0)
{
    return std::sqrt(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z);
}

template<typename T>
Vector3<T> invert(const Vector3<T>& v0)
{
    return { -v0.x, -v0.y, -v0.z };
}

template<typename T>
Vector3<T> normalize(const Vector3<T>& v0)
{
    return v0 / length(v0);
}

template<typename T>
Vector3<T> cross(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return {
        v0.y * v1.z - v0.z * v1.y,
        v0.z * v1.x - v0.x * v1.z,
        v0.x * v1.y - v0.y * v1.x
    };
}

template<typename T>
bool cwise_greater(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return v0.x > v1.x && v0.y > v1.y && v0.z > v1.z;
}

template<typename T>
bool cwise_greater_or_equal(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return v0.x >= v1.x && v0.y >= v1.y && v0.z >= v1.z;
}

template<typename T>
bool cwise_less(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return v0.x < v1.x && v0.y < v1.y && v0.z < v1.z;
}

template<typename T>
bool cwise_less_or_equal(const Vector3<T>& v0, const Vector3<T>& v1)
{
    return v0.x <= v1.x&& v0.y <= v1.y&& v0.z <= v1.z;
}

template<typename T>
Vector3<T> cwise_min(const Vector3<T>* v0, const Vector3<T>* v1)
{
    return {
        v0->x < v1->x ? v0->x : v1->x,
        v0->y < v1->y ? v0->y : v1->y,
        v0->z < v1->z ? v0->z : v1->z
    };
}

template<typename T>
Vector3<T> cwise_max(const Vector3<T>* v0, const Vector3<T>* v1)
{
    float x = v0->x > v1->x ? v0->x : v1->x;
    float y = v0->y > v1->y ? v0->y : v1->y;
    float z = v0->z > v1->z ? v0->z : v1->z;
    return {
        x, y, z
    };
}

template<typename T>
T radians(T x_in_degrees)
{
    return (x_in_degrees * std::numbers::pi_v<T>) / static_cast<T>(180);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vector3<T>& vec)
{
    return os << vec.x << ", " << vec.y << ", " << vec.z;
}

//
// Vector4
//
template<typename T>
struct Vector4
{
    Vector4()
        : x(0.f), y(0.f), z(0.f), w(0.f)
    {}

    Vector4(T val)
        : x(val), y(val), z(val), w(val)
    {}

    Vector4(T xx, T yy, T zz, T ww)
        : x(xx), y(yy), z(zz), w(ww)
    {}

    Vector4(const Vector3<T>& other)
        : x(other.x), y(other.y), z(other.z)
    {}

    Vector4(Vector3<T>&& other)
        : x(other.x), y(other.y), z(other.z), w(other.w)
    {}

    Vector4<T>& operator=(const Vector4<T>& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    Vector4<T>& operator=(Vector4<T>&& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }

    bool operator==(const Vector4<T>& other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    bool operator!=(const Vector4<T>& other) const
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

    Vector4<T> operator+(const Vector4<T>& v0) const
    {
        return { x + v0.x, y + v0.y, z + v0.z, w + v0.w };
    }

    Vector4<T> operator-(const Vector4<T>& v0) const
    {
        return { x - v0.x, y - v0.y, z - v0.z, w - v0.w };
    }

    Vector4<T> operator*(const Vector4<T>& v0) const
    {
        return { x * v0.x, y * v0.y, z * v0.z, w + v0.w };
    }

    Vector4<T> operator*(const T val) const
    {
        return { x * val, y * val, z * val, w * val };
    }

    Vector4<T> operator/(const T val) const
    {
        return this->operator*(1.f / val);
    }

    Vector4<T>& operator+=(const Vector4<T>& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    Vector4<T>& operator-=(const Vector4<T>& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    Vector4<T>& operator*=(const Vector4<T>& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }

    Vector4<T>& operator*=(const T val)
    {
        x *= val;
        y *= val;
        z *= val;
        w *= val;
        return *this;
    }

    Vector4<T>& operator/=(const T val)
    {
        x /= val;
        y /= val;
        z /= val;
        w /= val;
        return *this;
    }

    union
    {
        struct
        {
            T x, y, z, w;
        };
        T v[4];
    };
};

template<typename T>
Vector4<T> operator*(const T t, const Vector4<T>& v)
{
    return { v.x * t, v.y * t, v.z * t, v.w * t };
}

template<typename T>
T dot(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

template<typename T>
T length(const Vector4<T>& v0)
{
    return std::sqrt(v0.x * v0.x + v0.y * v0.y + v0.z * v0.z + v0.w * v0.w);
}

template<typename T>
Vector4<T> invert(const Vector4<T>& v0)
{
    return { -v0.x, -v0.y, -v0.z, -v0.w };
}

template<typename T>
Vector4<T> normalize(const Vector4<T>& v0)
{
    return v0 / length(v0);
}

template<typename T>
bool cwise_greater(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return v0.x > v1.x && v0.y > v1.y && v0.z > v1.z && v0.w > v1.w;
}

template<typename T>
bool cwise_greater_or_equal(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return v0.x >= v1.x && v0.y >= v1.y && v0.z >= v1.z && v0.w >= v1.w;
}

template<typename T>
bool cwise_less(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return v0.x < v1.x && v0.y < v1.y && v0.z < v1.z && v0.w < v1.w;
}

template<typename T>
bool cwise_less_or_equal(const Vector4<T>& v0, const Vector4<T>& v1)
{
    return v0.x <= v1.x && v0.y <= v1.y && v0.z <= v1.z && v0.w <= v1.w;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vector4<T>& vec)
{
    return os << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w;
}

}