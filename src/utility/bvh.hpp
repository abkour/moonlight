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

    void build_bvh(
        const float* tris,
        const uint64_t stride_in_bytes,
        uint32_t n_triangles
    );

    IntersectionParams intersect(
        Ray& ray, 
        const float* tris,
        const uint64_t stride_in_bytes
    );

    void to_file_ascii(const std::string& filename);

    void deserialize(const std::string& filename);
    void serialize(const std::string& filename);

    bool validate_parent_bigger_than_child();
    bool validate_all_bvs_well_defined();

    BVHNode* get_raw_nodes()
    {
        return m_bvh_nodes.get();
    }

    uint32_t* get_raw_indices()
    {
        return m_tri_idx.get();
    }

    unsigned get_nodes_used() const
    {
        return m_nodes_used;
    }

    unsigned get_num_nodes() const
    {
        return m_num_nodes;
    }

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

    std::unique_ptr<uint32_t[]> m_tri_idx;
    std::unique_ptr<BVHNode[]> m_bvh_nodes;
    unsigned m_num_nodes;
    unsigned m_nodes_used = 1;
};

}