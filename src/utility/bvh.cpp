#include "bvh.hpp"

namespace moonlight
{

void BVH::build_bvh(
    const Vector3<float>* tris,
    uint32_t n_triangles)
{
    std::unique_ptr<Vector3<float>[]> tri_centroids =
        std::make_unique<Vector3<float>[]>(n_triangles);

    m_bvh_nodes = std::make_unique<BVHNode[]>(n_triangles * 2 - 1);
    tri_idx     = std::make_unique<uint32_t[]>(n_triangles);

    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        tri_idx[i] = i;
    }

    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        tri_centroids[i] = 
            (tris[i * 3] + tris[i * 3 + 1] + tris[i * 3 + 2]) * 0.3333333f;
    }

    BVHNode& root = m_bvh_nodes[root_nodeidx];
    root.left_first = 0;
    root.tri_count = n_triangles;

    update_node_bounds(root_nodeidx, (Triangle*)tris);
    sub_divide(root_nodeidx, (Triangle*)tris, tri_centroids.get());
}

void BVH::update_node_bounds(
    uint32_t node_idx, 
    Triangle* tris)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    Vector3<float> bmin(std::numeric_limits<float>::max());
    Vector3<float> bmax(std::numeric_limits<float>::min());

    for (int i = 0; i < node.tri_count; ++i) 
    {
        unsigned leaf_tri_idx = tri_idx[node.left_first + i];
        const Triangle& leaf_tri = tris[leaf_tri_idx];
        node.aabbmin = cwise_min(node.aabbmin, leaf_tri.v0);
        node.aabbmin = cwise_min(node.aabbmin, leaf_tri.v1);
        node.aabbmin = cwise_min(node.aabbmin, leaf_tri.v2);
        node.aabbmax = cwise_max(node.aabbmax, leaf_tri.v0);
        node.aabbmax = cwise_max(node.aabbmax, leaf_tri.v1);
        node.aabbmax = cwise_max(node.aabbmax, leaf_tri.v2);
    }
}

void BVH::sub_divide(
    uint32_t node_idx,
    Triangle* tris,
    Vector3<float>* tri_centroids)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    if (node.tri_count <= 2) return;

    Vector3<float> extent = node.aabbmax - node.aabbmin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    float split_pos = node.aabbmin[axis] + extent[axis] * 0.5f;

    // Sort the primitives, such that primitives belonging to
    // group A are all in consecutive order.
    int i = node.left_first;
    int j = i + node.tri_count - 1;
    while (i <= j)
    {
        if (tri_centroids[tri_idx[i]][axis] < split_pos)
        {
            ++i;
        } else
        {
            std::swap(tri_idx[i], tri_idx[j--]);
        }
    }

    int left_count = i - node.left_first;
    if (left_count == 0 || left_count == node.tri_count)
    {
        return;
    }

    int left_child_idx = nodes_used++;
    int right_child_idx = nodes_used++;
    m_bvh_nodes[left_child_idx].left_first = node.left_first;
    m_bvh_nodes[left_child_idx].tri_count = left_count;
    m_bvh_nodes[right_child_idx].left_first = i;
    m_bvh_nodes[right_child_idx].tri_count = node.tri_count - left_count;
    node.left_first = left_child_idx;
    node.tri_count = 0;

    update_node_bounds(left_child_idx, tris);
    update_node_bounds(right_child_idx, tris);

    sub_divide(left_child_idx, tris, tri_centroids);
    sub_divide(right_child_idx, tris, tri_centroids);
}


void BVH::intersect(
    const Ray& ray, 
    const Vector3<float>* tris,
    IntersectionParams& intersect,
    const unsigned node_idx)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    float near_t = 0.f;
    float far_t  = 0.f;
    if (!ray_intersects_aabb(reinterpret_cast<AABB*>(&node.aabbmin), &ray, near_t, far_t))
    {
        return;
    }

    if (node.is_leaf())
    {
        IntersectionParams new_intersect;
        for (unsigned i = 0; i < node.tri_count; ++i)
        {
            new_intersect = ray_hit_triangle(ray, &tris[tri_idx[node.left_first + i]]);
            if (new_intersect.t < intersect.t)
            {
                memcpy(&intersect, &new_intersect, sizeof(IntersectionParams));
            }
        }
    }
    else
    {
        this->intersect(ray, tris, intersect, node.left_first);
        this->intersect(ray, tris, intersect, node.left_first + 1);
    }
}

}