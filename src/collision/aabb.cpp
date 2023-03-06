#include "aabb.hpp"

namespace moonlight
{

AABB aabb_construct_from_points(
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

float aabb_area(const AABB& aabb)
{
    Vector3<float> e = aabb.bmax - aabb.bmin;
    return e.x * e.y + e.y * e.z + e.x * e.z;
}

Vector3<float> aabb_center(const AABB& aabb)
{
    return (aabb.bmax + aabb.bmin) / 2.f;
}

bool aabb_empty(const AABB& x)
{
    return x.bmin == x.bmax;
}

void aabb_extend(AABB& aabb, const Vector3<float>& p)
{
    aabb.bmin = cwise_min(aabb.bmin, p);
    aabb.bmax = cwise_max(aabb.bmax, p);
}

bool aabb_degenerate(const AABB& x)
{
    return !cwise_less(x.bmin, x.bmax);
}

float aabb_volume(const AABB& x)
{
    Vector3<float> diff = x.bmax - x.bmin;
    return diff.x * diff.y * diff.z;
}

//
// Instanced AABBs
//
void construct_instanced_aabbs(
    AABB256* aabbs, uint32_t n_aabbs256,
    const float* inputvertices,
    const uint32_t n_input_vertices,
    const uint32_t input_vertex_format_stride,
    const float* instance_data,
    const uint32_t instance_data_stride)
{
    AABB os_aabb = aabb_construct_from_points(
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