#pragma once
#include "../simple_math.hpp"
#include <limits>

namespace moonlight
{

struct AABB
{
    Vector3<float> bmin, bmax;
};

inline AABB construct_aabb_from_points(
    const float* points,
    const std::size_t n_points, 
    const uint32_t stride)
{
    constexpr float lf = std::numeric_limits<float>::min();
    constexpr float hf = std::numeric_limits<float>::max();
    AABB aabb;
    aabb.bmin = { hf, hf, hf };
    aabb.bmax = { lf, lf, lf };

    for (std::size_t i = 0; i < n_points; ++i)
    {
        Vector3<float>* point_vertex = 
            (Vector3<float>*)((uint8_t*)points + (i * stride));
        if (point_vertex->x < aabb.bmin.x) aabb.bmin.x = point_vertex->x;
        if (point_vertex->y < aabb.bmin.y) aabb.bmin.y = point_vertex->y;
        if (point_vertex->z < aabb.bmin.z) aabb.bmin.z = point_vertex->z;

        if (point_vertex->x > aabb.bmax.x) aabb.bmax.x = point_vertex->x;
        if (point_vertex->y > aabb.bmax.y) aabb.bmax.y = point_vertex->y;
        if (point_vertex->z > aabb.bmax.z) aabb.bmax.z = point_vertex->z;
    }

    return aabb;
}

inline bool is_empty(const AABB& x)
{
    return x.bmin == x.bmax;
}

inline bool is_degenerate(const AABB& x)
{
    return !cwise_less(x.bmin, x.bmax);
}

inline float volume(const AABB& x)
{
    Vector3<float> diff = x.bmax - x.bmin;
    return diff.x * diff.y * diff.z;
}

struct alignas(32) AABB256
{
    float bmin_x[8];
    float bmax_x[8];
    float bmin_y[8];
    float bmax_y[8];
    float bmin_z[8];
    float bmax_z[8];
};

inline void construct_instanced_aabbs(
    AABB256* aabbs, uint32_t n_aabbs256,
    const float* inputvertices,
    const uint32_t n_input_vertices,
    const uint32_t input_vertex_format_stride,
    const float* instance_data,
    const uint32_t instance_data_stride)
{
    AABB os_aabb = construct_aabb_from_points(
        inputvertices,
        n_input_vertices,
        input_vertex_format_stride
    );

    for (int j = 0; j < n_aabbs256; ++j)
    {
        for (int k = 0; k < 8; ++k)
        {
            int i = j * 8 + k;
            const Vector3<float>* displacement = 
                (Vector3<float>*)((uint8_t*)instance_data + (i * instance_data_stride));
            aabbs[j].bmin_x[k] = os_aabb.bmin.x + displacement->x;
            aabbs[j].bmin_y[k] = os_aabb.bmin.y + displacement->y;
            aabbs[j].bmin_z[k] = os_aabb.bmin.z + displacement->z;
            aabbs[j].bmax_x[k] = os_aabb.bmax.x + displacement->x;
            aabbs[j].bmax_y[k] = os_aabb.bmax.y + displacement->y;
            aabbs[j].bmax_z[k] = os_aabb.bmax.z + displacement->z;
        }
    }
}

}