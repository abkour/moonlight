#include "bvh.hpp"
#include <fstream>
#include <Windows.h>

namespace moonlight
{

void BVH::build_bvh(
    const Vector3<float>* tris,
    uint32_t n_triangles)
{
    n_nodes = n_triangles * 2 - 1;

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

    const unsigned root_nodeidx = 0;
    BVHNode& root = m_bvh_nodes[root_nodeidx];
    root.left_first = 0;
    root.tri_count = n_triangles;

    update_node_bounds(root_nodeidx, (Triangle*)tris);
    sub_divide(root_nodeidx, (Triangle*)tris, tri_centroids.get());
}

void BVH::update_node_bounds(
    uint32_t node_idx, 
    const Triangle* tris)
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
    const Triangle* tris,
    const Vector3<float>* tri_centroids)
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
    int j = node.left_first + node.tri_count - 1;
    while (i <= j)
    {
        auto& centroid = tri_centroids[tri_idx[i]];
        if (centroid[axis] < split_pos)
        {
            ++i;
        } 
        else
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


static bool IntersectAABB(const Ray& ray, const Vector3<float> bmin, const Vector3<float> bmax)
{
    float tx1 = (bmin.x - ray.o.x) / ray.d.x;
    float tx2 = (bmax.x - ray.o.x) / ray.d.x;
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y - ray.o.y) / ray.d.y;
    float ty2 = (bmax.y - ray.o.y) / ray.d.y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z - ray.o.z) / ray.d.z;
    float tz2 = (bmax.z - ray.o.z) / ray.d.z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

void BVH::intersect(
    Ray& ray, 
    const Vector3<float>* tris,
    IntersectionParams& intersect,
    const unsigned node_idx)
{
    const BVHNode& node = m_bvh_nodes[node_idx];
    /*if (!ray_intersects_aabb((AABB*)(&node.aabbmin), &ray))
    {
        return;
    }*/
    if (!IntersectAABB(ray, node.aabbmin, node.aabbmax))
    {
        return;
    }

    if (node.is_leaf())
    {
        for (unsigned i = 0; i < node.tri_count; ++i)
        {
            IntersectionParams new_intersect 
                = ray_hit_triangle(ray, &tris[tri_idx[node.left_first + i] * 3]);
            if (new_intersect.t < ray.t)
            {
                ray.t = new_intersect.t;
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

void BVH::deserialize(const char* filename)
{
    std::ifstream file(filename, std::ios::binary);

    file.read((char*)&n_nodes, sizeof(unsigned));
    const unsigned n_triangles = (n_nodes + 1) / 2;
    
    tri_idx = std::make_unique<unsigned[]>(n_triangles);
    m_bvh_nodes = std::make_unique<BVHNode[]>(n_nodes);

    file.read((char*)tri_idx.get(), sizeof(unsigned) * n_triangles);
    file.read((char*)m_bvh_nodes.get(), sizeof(BVHNode) * n_nodes);
    file.read((char*)&nodes_used, sizeof(unsigned));
}

void BVH::serialize(const char* filename)
{
    const unsigned n_triangles = (n_nodes + 1) / 2;

    std::ofstream file(filename, std::ios::binary);
    file.write((char*)&n_nodes, sizeof(unsigned));
    file.write((char*)tri_idx.get(), sizeof(unsigned) * n_triangles);
    file.write((char*)m_bvh_nodes.get(), sizeof(BVHNode) * n_nodes);
    file.write((char*)&nodes_used, sizeof(unsigned));
}

}