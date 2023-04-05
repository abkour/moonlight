#pragma once
#include "material.hpp"
#include "../../utility/bvh.hpp"
#include "../../project_defines.hpp"
#include "../../simple_math.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace moonlight
{

struct Model
{
    Model();
    ~Model();

    void build_bvh();

    void parse_mof(const std::string& filename);

    // Most important functions
    IntersectionParams intersect(Ray& ray) const;
    IMaterial* get_material(uint32_t material_idx) const
    {
        return m_materials[material_idx];
    }

    uint32_t bvh_nodes_used() const
    {
        return m_bvh->get_nodes_used();
    }

    uint32_t bvh_total_num_nodes() const
    {
        return m_bvh->get_num_nodes();
    }

    uint32_t* bvh_get_raw_indices()
    {
        return m_bvh->get_raw_indices();
    }

    BVHNode* bvh_get_raw_nodes()
    {
        return m_bvh->get_raw_nodes();
    }

    void bvh_deserialize(const std::string& filename)
    {
        m_bvh->deserialize(filename.c_str());
    }

    void bvh_serialize(const std::string& filename)
    {
        std::string new_filename = filename + ".bvh";
        m_bvh->serialize(new_filename.c_str());
    }

    Vector3<float> color_rgb(const uint32_t material_idx) const;
    Vector4<float> color_rgba(const uint32_t material_idx) const;

    uint64_t material_flags() const
    {
        return m_mesh_flags;
    }

    uint32_t material_idx(IntersectionParams& intersect) const;

    Vector3<float> normal(uint32_t triangle_idx) const;

    uint64_t num_elements() const
    {
        return m_mesh_num_elements;
    }

    uint64_t num_triangles() const
    {
        return m_num_triangles;
    }

    float* raw_mesh()
    {
        return m_mesh.get();
    }

    uint64_t stride() const
    {
        return m_stride_in_32floats;
    }

private:

    std::unique_ptr<BVH> m_bvh;

    uint64_t m_num_triangles;
    uint64_t m_stride_in_32floats;
    uint64_t m_mesh_flags;

    uint64_t m_num_materials;
    std::vector<IMaterial*> m_materials;
    std::vector<ITexture*> m_textures;

    std::unique_ptr<float[]> m_mesh;
    size_t m_mesh_num_elements;
};

}