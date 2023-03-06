#include "bvh.hpp"
#include <fstream>
#include <Windows.h>

#include "../logging_file.hpp"

namespace moonlight
{

void BVH::build_bvh(const Triangle* tris, uint32_t n_triangles)
{
    n_nodes = n_triangles * 2 - 1;

    m_bvh_nodes = std::make_unique<BVHNode[]>(n_nodes);
    tri_idx     = std::make_unique<uint32_t[]>(n_triangles);

    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        tri_idx[i] = i;
    }
    
    BVHNode& root = m_bvh_nodes[0];
    root.left_first = 0;
    root.tri_count = n_triangles;

    update_node_bounds(0, tris);
    sub_divide(0, tris);
}

void BVH::update_node_bounds(uint32_t node_idx, const Triangle* tris)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    
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

void BVH::sub_divide(uint32_t node_idx, const Triangle* tris)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    int axis;
    float split_pos;
    if (!compute_optimal_split(node, tris, axis, split_pos))
        return;

    // Sort the primitives, such that primitives belonging to
    // group A are all in consecutive order.
    int i = node.left_first;
    int j = node.left_first + node.tri_count - 1;
    while (i <= j)
    {
        if (tris[tri_idx[i]].centroid[axis] < split_pos)
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

    sub_divide(left_child_idx, tris);
    sub_divide(right_child_idx, tris);
}

bool BVH::compute_optimal_split(
    const BVHNode& node, const Triangle* tris, int& axis, float& split_pos)
{
    Vector3<float> e = node.aabbmax - node.aabbmin; // extent of parent
    float parent_area = e.x * e.y + e.y * e.z + e.z * e.x;
    float parent_cost = node.tri_count * parent_area;

    int best_axis = -1;
    float best_pos = 0.f;
    float best_cost = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis)
    {
        for (unsigned i = 0; i < node.tri_count; ++i)
        {
            unsigned idx = tri_idx[node.left_first + i];
            float candidate_pos = tris[idx].centroid[axis];
            float cost = compute_sah(node, tris, axis, candidate_pos);
            if (cost < best_cost)
            {
                best_pos = candidate_pos;
                best_axis = axis;
                best_cost = cost;
            }
        }
    }

    if (best_cost >= parent_cost) return false;
    
    axis = best_axis;
    split_pos = best_pos;
    return true;
}

static float IntersectAABB(const Ray& ray, const Vector3<float> bmin, const Vector3<float> bmax)
{
    float tx1 = (bmin.x - ray.o.x) * ray.invd.x;
    float tx2 = (bmax.x - ray.o.x) * ray.invd.x;
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);
    float ty1 = (bmin.y - ray.o.y) * ray.invd.y;
    float ty2 = (bmax.y - ray.o.y) * ray.invd.y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    float tz1 = (bmin.z - ray.o.z) * ray.invd.z;
    float tz2 = (bmax.z - ray.o.z) * ray.invd.z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));
    
    if (tmax >= tmin && tmin < ray.t && tmax > 0)
    {
        return tmin;
    }
    else
    {
        return std::numeric_limits<float>::max();
    }
}

static float IntersectAABB_SSE(const Ray& ray, const __m128 bmin4, const __m128 bmax4)
{
    static __m128 mask4 = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_set_ps(1, 0, 0, 0));
    __m128 t1 = _mm_mul_ps(_mm_sub_ps(_mm_and_ps(bmin4, mask4), ray.o4), ray.invd4);
    __m128 t2 = _mm_mul_ps(_mm_sub_ps(_mm_and_ps(bmax4, mask4), ray.o4), ray.invd4);
    __m128 vmax4 = _mm_max_ps(t1, t2), vmin4 = _mm_min_ps(t1, t2);
    float tmax = std::min(vmax4.m128_f32[0], std::min(vmax4.m128_f32[1], vmax4.m128_f32[2]));
    float tmin = std::max(vmin4.m128_f32[0], std::max(vmin4.m128_f32[1], vmin4.m128_f32[2]));
    
    if (tmax >= tmin && tmin < ray.t && tmax > 0) 
        return tmin; 
    else 
        return std::numeric_limits<float>::max();
}

void BVH::intersect(
    Ray& ray, const Triangle* tris, IntersectionParams& intersect)
{
    const BVHNode* node = &m_bvh_nodes[0];
    BVHNode* stack[64];
    unsigned stack_ptr = 0;

    while (true)
    {
        if (node->is_leaf())
        {
            for (unsigned i = 0; i < node->tri_count; ++i)
            {
                IntersectionParams new_intersect = ray_hit_triangle(
                    ray, 
                    (Vector3<float>*)&tris[tri_idx[node->left_first + i]]
                );

                if (new_intersect.t < ray.t)
                {
                    ray.t = new_intersect.t;
                }
            }

            if (stack_ptr == 0)
            {
                break;
            }
            else
            {
                node = stack[--stack_ptr];
            }
            
            continue;
        }

        BVHNode* child1 = &m_bvh_nodes[node->left_first];
        BVHNode* child2 = &m_bvh_nodes[node->left_first + 1];

        float dist1 = IntersectAABB(ray, child1->aabbmin, child1->aabbmax);
        float dist2 = IntersectAABB(ray, child2->aabbmin, child2->aabbmax);

        if (dist1 > dist2)
        {
            std::swap(dist1, dist2);
            std::swap(child1, child2);
            // After this, child1 is always the nearest node and child2 is always the farthest node
            // child1 : child_near
            // child2 : child_far
        }

        // If the near child has not been hit, then neither node has been hit
        if (dist1 == std::numeric_limits<float>::max())
        {
            if (stack_ptr == 0)
            {
                // Since there are no more nodes to process, and the ray has missed
                // both bvs, we finish.
                break;
            }
            else
            {
                node = stack[--stack_ptr];
            }
        }
        else
        {
            // The near node has been hit, we will process it in the next
            // loop iteration
            node = child1;
            if (dist2 != std::numeric_limits<float>::max())
            {
                // The far node has been hit as well, we will store it in the stack
                // for later processing.
                stack[stack_ptr++] = child2;
            }
        }
    }
}

float BVH::compute_sah(
    const BVHNode& node, 
    const Triangle* tris, 
    int axis, 
    float pos)
{
    AABB left_box;
    AABB right_box;
    int left_count = 0; 
    int right_count = 0;

    unsigned idx = 0;
    for (unsigned i = 0; i < node.tri_count; ++i)
    {
        idx = tri_idx[node.left_first + i];
        const Triangle& triangle = tris[idx];
        if (triangle.centroid[axis] < pos)
        {
            left_count++;
            aabb_extend(left_box, triangle.v0);
            aabb_extend(left_box, triangle.v1);
            aabb_extend(left_box, triangle.v2);
        }
        else
        {
            right_count++;
            aabb_extend(right_box, triangle.v0);
            aabb_extend(right_box, triangle.v1);
            aabb_extend(right_box, triangle.v2);
        }
    }

    float cost = left_count * aabb_area(left_box) + right_count * aabb_area(right_box);
    return cost > 0 ? cost : std::numeric_limits<float>::max();
}

bool BVH::validate_parent_bigger_than_child()
{
    for (int i = 0; i < nodes_used; ++i)
    {
        const BVHNode* node = &m_bvh_nodes[i];
        if (!node->is_leaf())
        {
            const BVHNode& child1 = m_bvh_nodes[node->left_first];
            const BVHNode& child2 = m_bvh_nodes[node->left_first + 1];

            if (cwise_greater(node->aabbmin, child1.aabbmin) || 
                cwise_greater(node->aabbmin, child2.aabbmin))
            {
                return false;
            }

            if (cwise_less(node->aabbmax, child1.aabbmax) ||
                cwise_less(node->aabbmax, child2.aabbmax))
            {
                return false;
            }
        }
    }
}

bool BVH::validate_all_bvs_well_defined()
{
    const unsigned n_interiors = n_nodes;
    
    const Vector3<float> empty_vector(0.f);
    for (int i = 0; i < n_nodes; ++i)
    {
        const BVHNode* node = &m_bvh_nodes[i];
        if (node->aabbmin == node->aabbmax)
        {
            if (node->aabbmin == empty_vector)
            {
                return false;
            }
        }
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