#pragma once
#include "../simple_math.hpp"
#include "../collision/primitive_tests.hpp"
#include "../collision/ray.hpp"

namespace moonlight
{

struct BVHNode
{
    BVHNode()
        : aabbmin(std::numeric_limits<float>::max())
        , aabbmax(-std::numeric_limits<float>::max())
        , left_first(0)
        , tri_count(0)
    {}

    bool is_leaf() const
    {
        return tri_count > 0;
    }

    union
    {
        struct { Vector3<float> aabbmin; unsigned int left_first; };
        __m128 aabbMin4;
    };

    union
    {
        struct { Vector3<float> aabbmax; unsigned int tri_count; };
        __m128 aabbMax4;
    };
};

class BVH
{
public:

    ~BVH();

    void build_bvh(
        const float* tris,
        const uint64_t stride_in_bytes,
        uint32_t n_triangles
    );

    void intersect(
        Ray& ray, 
        const float* tris,
        const uint64_t stride_in_bytes,
        IntersectionParams& intersect
    );

    void deserialize(const char* filename);
    void serialize(const char* filename);

    bool validate_parent_bigger_than_child();
    bool validate_all_bvs_well_defined();

private:

    void update_node_bounds(uint32_t node_idx, const float* tris, const uint64_t stride);

    void sub_divide(uint32_t node_idx, const float* tris, const uint64_t stride);

    float compute_sah(
        const BVHNode& node, const float* tris, 
        const uint64_t stride, int axis, float pos
    );

    bool compute_optimal_split(
        const BVHNode& node, const float* tris, 
        const uint64_t stride, int& axis, float& split_pos
    );

    unsigned compute_triangle_pos(
        unsigned triangle_pos, unsigned stride
    );

private:

    std::unique_ptr<uint32_t[]> tri_idx;
    std::unique_ptr<BVHNode[]> m_bvh_nodes;
    unsigned n_nodes;
    unsigned nodes_used = 1;
    unsigned loopc = 0;
};

}