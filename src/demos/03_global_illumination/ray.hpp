#pragma once
#include "../../simple_math.hpp"
#include <ostream>

namespace moonlight {

// Represents a ray in world space
struct Ray {

    // Default constructed rays are meaningless. However, they are needed for intermediate steps.
    Ray() = default;
    // Constructs a ray with origin \origin and direction \direction. Must make sure that the 
    // direction vector is normalized
    Ray(const Vector3& origin, const Vector3& direction);
    // Copy construct ray
    Ray(const Ray& other);
    // Copy assign ray
    Ray& operator=(const Ray& other);

    // Compute point the ray travelled to at time point \t. 
    Vector3 operator()(const float t);

    // origin refers to the ray origin in world space and direction refers to the direction
    // the ray travels in. The direction vector has to be normalized
    Vector3 o, d;
    Vector3 invd;	// Inverse direction is used for some intersection algorithms for performance
};

// Print out the parameters of the ray
std::ostream& operator<<(std::ostream& os, const Ray& ray);

}