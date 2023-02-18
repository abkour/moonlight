#pragma once
#include "../simple_math.hpp"
#include <limits>

namespace moonlight
{

struct AABB
{
    Vector3 bmin, bmax;
};

inline AABB construct_aabb_from_points(const float* points, const uint32_t stride, const std::size_t n_points)
{
    constexpr float lf = std::numeric_limits<float>::min();
    constexpr float hf = std::numeric_limits<float>::max();
    AABB aabb;
    aabb.bmin = { hf, hf, hf };
    aabb.bmax = { lf, lf, lf };

    for (std::size_t i = 0; i < n_points; ++i)
    {
        Vector3* point_vertex = (Vector3*)((uint8_t*)points + (i * stride));
        if (point_vertex[i].x < aabb.bmin.x) aabb.bmin.x = point_vertex[i].x;
        if (point_vertex[i].y < aabb.bmin.y) aabb.bmin.y = point_vertex[i].y;
        if (point_vertex[i].z < aabb.bmin.z) aabb.bmin.z = point_vertex[i].z;

        if (point_vertex[i].x > aabb.bmax.x) aabb.bmax.x = point_vertex[i].x;
        if (point_vertex[i].y > aabb.bmax.y) aabb.bmax.y = point_vertex[i].y;
        if (point_vertex[i].z > aabb.bmax.z) aabb.bmax.z = point_vertex[i].z;
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
    Vector3 diff = x.bmax - x.bmin;
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
    const float* cube_vertices,
    const uint32_t n_cube_vertices,
    const uint32_t cube_vertex_format_stride,
    const InstanceAttributes* instance_data)
{
    AABB os_aabb = construct_aabb_from_points(
        cube_vertices,
        cube_vertex_format_stride,
        n_cube_vertices
    );

    for (int j = 0; j < n_aabbs256; ++j)
    {
        for (int k = 0; k < 8; ++k)
        {
            int i = j * 8 + k;
            aabbs[j].bmin_x[k] = os_aabb.bmin.x + instance_data[i].displacement.x;
            aabbs[j].bmin_y[k] = os_aabb.bmin.y + instance_data[i].displacement.y;
            aabbs[j].bmin_z[k] = os_aabb.bmin.z + instance_data[i].displacement.z;
            aabbs[j].bmax_x[k] = os_aabb.bmax.x + instance_data[i].displacement.x;
            aabbs[j].bmax_y[k] = os_aabb.bmax.y + instance_data[i].displacement.y;
            aabbs[j].bmax_z[k] = os_aabb.bmax.z + instance_data[i].displacement.z;
        }
    }
}

}