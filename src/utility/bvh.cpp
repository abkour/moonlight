#include "bvh.hpp"
#include <fstream>
#include <Windows.h>

#include "../logging_file.hpp"

namespace moonlight
{

constexpr unsigned VERT_PER_TRIANGLE = 3;
constexpr unsigned MATERIAL_INDEX_SIZE = 1;

using vec3f = Vector3<float>;

// It is assumed that the triangle also contains a vec3f centroid at the
// end of the three vertices
unsigned BVH::compute_triangle_pos(
    unsigned triangle_pos, unsigned stride)
{
    return m_tri_idx[triangle_pos] 
        * (stride * VERT_PER_TRIANGLE + VERT_PER_TRIANGLE + MATERIAL_INDEX_SIZE);
}

void BVH::build_bvh(
    const float* tris, 
    const uint64_t stride_in_bytes,
    uint32_t n_triangles)
{
    m_num_nodes = n_triangles * 2 - 1;

    m_bvh_nodes = std::make_unique<BVHNode[]>(m_num_nodes);
    m_tri_idx   = std::make_unique<uint32_t[]>(n_triangles);

    for (uint32_t i = 0; i < n_triangles; ++i)
    {
        m_tri_idx[i] = i;
    }
    
    BVHNode& root   = m_bvh_nodes[0];
    root.left_first = 0;
    root.tri_count  = n_triangles;

    update_node_bounds(0, tris, stride_in_bytes);
    sub_divide(0, tris, stride_in_bytes);
}

void BVH::update_node_bounds(
    uint32_t node_idx, 
    const float* tris,
    const uint64_t stride)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    
    for (int i = 0; i < node.tri_count; ++i)
    {
        unsigned idx = compute_triangle_pos(i + node.left_first, stride);
        const float* leaf_tri = &tris[idx];
        node.aabbmin = cwise_min(&node.aabbmin, (vec3f*)&leaf_tri[0]);
        node.aabbmin = cwise_min(&node.aabbmin, (vec3f*)&leaf_tri[stride]);
        node.aabbmin = cwise_min(&node.aabbmin, (vec3f*)&leaf_tri[stride * 2]);
        node.aabbmax = cwise_max(&node.aabbmax, (vec3f*)&leaf_tri[0]);
        node.aabbmax = cwise_max(&node.aabbmax, (vec3f*)&leaf_tri[stride]);
        node.aabbmax = cwise_max(&node.aabbmax, (vec3f*)&leaf_tri[stride * 2]);
    }
}

void BVH::sub_divide(uint32_t node_idx, const float* tris, const uint64_t stride)
{
    BVHNode& node = m_bvh_nodes[node_idx];
    int axis;
    float split_pos;
    if (!compute_optimal_split(node, tris, stride, axis, split_pos))
        return;

    unsigned centroid_off = stride * VERT_PER_TRIANGLE;
    // Sort the primitives, such that primitives belonging to
    // group A are all in consecutive order.
    int i = node.left_first;
    int j = node.left_first + node.tri_count - 1;
    while (i <= j)
    {
        unsigned centroid_pos = compute_triangle_pos(i, stride) + centroid_off;
        if (tris[centroid_pos + axis] < split_pos)
        {
            ++i;
        } 
        else
        {
            std::swap(m_tri_idx[i], m_tri_idx[j--]);
        }
    }

    int left_count = i - node.left_first;
    if (left_count == 0 || left_count == node.tri_count)
    {
        return;
    }

    int left_child_idx = m_nodes_used++;
    int right_child_idx = m_nodes_used++;
    m_bvh_nodes[left_child_idx].left_first = node.left_first;
    m_bvh_nodes[left_child_idx].tri_count = left_count;
    m_bvh_nodes[right_child_idx].left_first = i;
    m_bvh_nodes[right_child_idx].tri_count = node.tri_count - left_count;
    node.left_first = left_child_idx;
    node.tri_count = 0;

    update_node_bounds(left_child_idx, tris, stride);
    update_node_bounds(right_child_idx, tris, stride);

    sub_divide(left_child_idx, tris, stride);
    sub_divide(right_child_idx, tris, stride);
}

bool BVH::compute_optimal_split(
    const BVHNode& node, 
    const float* tris, 
    const uint64_t stride,
    int& axis, 
    float& split_pos)
{
    const unsigned centroid_off = stride * VERT_PER_TRIANGLE;

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
            unsigned centroid_pos = 
                compute_triangle_pos(i + node.left_first, stride) + centroid_off;

            float candidate_pos = tris[centroid_pos + axis];
            float cost = compute_sah(node, tris, stride, axis, candidate_pos);
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

static float IntersectAABB(const Vector3<float> bmin, const Vector3<float> bmax, const Ray& ray)
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

static float IntersectAABB_SSE(const __m128 bmin4, const __m128 bmax4, const Ray& ray)
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

IntersectionParams BVH::intersect(
    Ray& ray, 
    const float* tris, 
    const uint64_t stride)
{
    IntersectionParams intersect;

    const BVHNode* node = &m_bvh_nodes[0];
    BVHNode* stack[64];
    unsigned stack_ptr = 0;

    const unsigned triangle_size = stride * VERT_PER_TRIANGLE + VERT_PER_TRIANGLE;

    while (true)
    {
        if (node->is_leaf())
        {
            for (unsigned i = 0; i < node->tri_count; ++i)
            {
                unsigned triangle_pos =
                    compute_triangle_pos(i + node->left_first, stride);

                IntersectionParams new_intersect = ray_hit_triangle(
                    ray, &tris[triangle_pos], stride
                );

                if (new_intersect.t < intersect.t && new_intersect.t > 0.f)
                {
                    intersect = new_intersect;
                    intersect.triangle_idx = triangle_pos;
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

        // TODO: There is a strange performance degradation when both distances 
        // are computed via the ray_intersects_aabb method. When only one of the
        // distances is computed with that function, the performance improves
        // over the IntersectAABB method. 
        // How can this behavior be explained? 
        //      - The number of loop iterations is not the culprit.
        float dist1 = IntersectAABB(child1->aabbmin, child1->aabbmax, ray);
        float dist2 = IntersectAABB(child2->aabbmin, child2->aabbmax, ray);
        //float dist2 = ray_intersects_aabb(child2->aabbmin, child2->aabbmax, ray);

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

    return intersect;
}


float BVH::compute_sah(
    const BVHNode& node, const float* tris, 
    const uint64_t stride,
    int axis, float pos)
{
    const unsigned centroid_offset = stride * VERT_PER_TRIANGLE;

    AABB left_box;
    AABB right_box;
    int left_count = 0; 
    int right_count = 0;
    
    for (unsigned i = 0; i < node.tri_count; ++i)
    {
        unsigned triangle_pos =
            compute_triangle_pos(i + node.left_first, stride);
        const float* triangle = &tris[triangle_pos];
        if (triangle[centroid_offset + axis] < pos)
        {
            left_count++;
            aabb_extend(&left_box, (Vector3<float>*)&triangle[0]);
            aabb_extend(&left_box, (Vector3<float>*)&triangle[stride]);
            aabb_extend(&left_box, (Vector3<float>*)&triangle[stride * 2]);
        }
        else
        {
            right_count++;
            aabb_extend(&right_box, (Vector3<float>*)&triangle[0]);
            aabb_extend(&right_box, (Vector3<float>*)&triangle[stride]);
            aabb_extend(&right_box, (Vector3<float>*)&triangle[stride * 2]);
        }
    }

    float cost = left_count * aabb_area(left_box) + right_count * aabb_area(right_box);
    return cost > 0 ? cost : std::numeric_limits<float>::max();
}

bool BVH::validate_parent_bigger_than_child()
{
    for (int i = 0; i < m_nodes_used; ++i)
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
    const Vector3<float> empty_vector(0.f);
    for (int i = 0; i < m_num_nodes; ++i)
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

    file.read((char*)&m_num_nodes, sizeof(unsigned));
    const unsigned n_triangles = (m_num_nodes + 1) / 2;
    
    m_tri_idx = std::make_unique<unsigned[]>(n_triangles);
    m_bvh_nodes = std::make_unique<BVHNode[]>(m_num_nodes);

    file.read((char*)m_tri_idx.get(), sizeof(unsigned) * n_triangles);
    file.read((char*)m_bvh_nodes.get(), sizeof(BVHNode) * m_num_nodes);
    file.read((char*)&m_nodes_used, sizeof(unsigned));
}

void BVH::serialize(const char* filename)
{
    const unsigned n_triangles = (m_num_nodes + 1) / 2;

    std::ofstream file(filename, std::ios::binary);
    file.write((char*)&m_num_nodes, sizeof(unsigned));
    file.write((char*)m_tri_idx.get(), sizeof(unsigned) * n_triangles);
    file.write((char*)m_bvh_nodes.get(), sizeof(BVHNode) * m_num_nodes);
    file.write((char*)&m_nodes_used, sizeof(unsigned));
}

void BVH::to_file_ascii(const char* filename)
{

}

}