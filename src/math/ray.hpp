#pragma once
#include "intersect.hpp"
#include "../simple_math.hpp"
#include <ostream>

namespace moonlight {

// Represents a ray in world space
struct Ray {

    // Default constructed rays are meaningless. However, they are needed for intermediate steps.
    Ray() = default;
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
    Vector3<float> o, d;
    Vector3<float> invd;	// Inverse direction is used for some intersection algorithms for performance
};

// Print out the parameters of the ray
std::ostream& operator<<(std::ostream& os, const Ray& ray);

IntersectionParams ray_hit_triangle(const Ray& ray, const Vector3<float>* tris);

}