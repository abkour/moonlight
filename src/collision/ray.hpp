#pragma once
#include "intersect.hpp"
#include "../simple_math.hpp"
#include <ostream>
#include <intrin.h>

namespace moonlight {

// Represents a ray in world space
struct Ray {

    // Default constructed rays are meaningless. However, they are needed for intermediate steps.
    Ray()
    {
        o4 = d4 = invd4 = _mm_set1_ps(1);
    }
    // Constructs a ray with origin \origin and direction \direction. Must make sure that the 
    // direction vector is normalized
    Ray(const Vector3<float>& origin, const Vector3<float>& direction);
    // Copy construct ray
    Ray(const Ray& other);
    // Copy assign ray
    Ray& operator=(const Ray& other);

    // Compute point the ray travelled to at time point \t. 
    Vector3<float> operator()(const float t);

    // origin refers to the ray origin in world space and direction refers to the direction
    // the ray travels in. The direction vector has to be normalized
    float t = std::numeric_limits<float>::max();
   
    union {
        struct {
            Vector3<float> o;
            float dummy1;
        };
        __m128 o4;
    };

    union {
        struct {
            Vector3<float> d;
            float dummy2;
        };
        __m128 d4;
    };

    union {
        struct {
            Vector3<float> invd;
            float dummy4;
        };
        __m128 invd4;
    };
};

// Print out the parameters of the ray
std::ostream& operator<<(std::ostream& os, const Ray& ray);

IntersectionParams ray_hit_triangle(
    const Ray& ray,  const float* tris, const unsigned stride
);

IntersectionParams ray_hit_triangle(
    const Ray& ray, 
    const float* tri0, const float* tri1, const float* tri2, 
    const unsigned stride
);

}