#pragma once
#include "../simple_math.hpp"
#include <limits>

namespace moonlight
{

struct AABB
{
    AABB()
        : bmin(std::numeric_limits<float>::max())
        , bmax(-std::numeric_limits<float>::max())
    {}

    Vector3<float> bmin, bmax;
};

AABB aabb_construct_from_points(
    const float* points,
    const std::size_t n_points,
    const uint32_t stride
);

float aabb_area(const AABB& aabb);

Vector3<float> aabb_center(const AABB& aabb);

void aabb_extend(AABB* aabb, const Vector3<float>* p);

bool aabb_empty(const AABB& x);

bool aabb_degenerate(const AABB& x);

float aabb_volume(const AABB& x);

struct alignas(32) AABB256
{
    float bmin_x[8];
    float bmax_x[8];
    float bmin_y[8];
    float bmax_y[8];
    float bmin_z[8];
    float bmax_z[8];
};

void construct_instanced_aabbs(
    AABB256* aabbs, uint32_t n_aabbs256,
    const float* inputvertices,
    const uint32_t n_input_vertices,
    const uint32_t input_vertex_format_stride,
    const float* instance_data,
    const uint32_t instance_data_stride
);

}