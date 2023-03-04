#pragma once
#include "../simple_math.hpp"
#include "../collision/primitive_tests.hpp"
#include "../collision/ray.hpp"

namespace moonlight
{

struct BVHNode
{
    bool is_leaf() const
    {
        return tri_count > 0;
    }

    Vector3<float> aabbmin, aabbmax;
    unsigned int left_first, tri_count;
};

class BVH
{
public:

    void build_bvh(
        const Vector3<float>* tris,
        uint32_t n_triangles
    );

    void intersect(
        Ray& ray, 
        const Vector3<float>* tris, 
        IntersectionParams& intersect, 
        const unsigned node_idx = 0
    );

    void deserialize(const char* filename);
    void serialize(const char* filename);

private:

    struct Triangle
    {
        Vector3<float> v0;
        Vector3<float> v1;
        Vector3<float> v2;
    };

    void update_node_bounds(
        uint32_t node_idx,
        const Triangle* tris
    );

    void sub_divide(
        uint32_t node_idx, 
        const Triangle* tris,
        const Vector3<float>* tri_centroids
    );

private:

    std::unique_ptr<uint32_t[]> tri_idx;
    std::unique_ptr<BVHNode[]> m_bvh_nodes;
    unsigned n_nodes;
    unsigned nodes_used = 1;
};

}