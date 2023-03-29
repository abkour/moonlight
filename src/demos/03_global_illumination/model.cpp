#include "model.hpp"
#include <fstream>

#include "material_lambertian.hpp"
#include "texture_single.hpp"

namespace moonlight
{

Model::Model(const char* filename)
{
    uint64_t n_attr_data_bytes = 0;

    std::ifstream file(filename, std::ios::in | std::ios::binary);

    file.read((char*)&m_num_triangles, sizeof(uint64_t));
    file.read((char*)&m_stride_in_32floats, sizeof(uint64_t));
    file.read((char*)&n_attr_data_bytes, sizeof(uint64_t));
    file.read((char*)&m_mesh_flags, sizeof(uint64_t));

    m_mesh = std::make_unique<float[]>(n_attr_data_bytes / sizeof(float));

    file.read((char*)m_mesh.get(), n_attr_data_bytes);
    m_mesh_num_elements = n_attr_data_bytes / sizeof(float);

    if (m_mesh_flags & ML_MISC_FLAG_MATERIALS_APPENDED)
    {
        file.read((char*)&m_num_materials, sizeof(uint64_t));
        
        m_materials.resize(m_num_materials);
        m_textures.resize(m_num_materials);
        
        m_mat_diffuse_colors = std::make_unique<Vector3<float>[]>(m_num_materials);
        file.read((char*)m_mat_diffuse_colors.get(), sizeof(Vector3<float>) * m_num_materials);
    
        for (int i = 0; i < m_num_materials; ++i)
        {
            m_textures[i] = new SingleColor(
                m_mat_diffuse_colors[i].x, 
                m_mat_diffuse_colors[i].y, 
                m_mat_diffuse_colors[i].z, 
                1.f
            );

            m_materials[i] = new LamberrtianMaterial(m_textures[i]);
        }
    }
}

void Model::build_bvh()
{
    m_bvh = std::make_unique<BVH>();
    m_bvh->build_bvh(m_mesh.get(), m_stride_in_32floats, m_num_triangles);
}

Vector3<float> Model::color_rgb(const uint32_t material_idx)
{
    Vector4<float> color = m_textures[material_idx]->color(0.f, 0.f);
    Vector3<float> res(color.x, color.y, color.z);
    return res;
}

Vector4<float> Model::color_rgba(const uint32_t material_idx)
{
    return m_textures[material_idx]->color(0.f, 0.f);
}

IntersectionParams Model::intersect(Ray& ray)
{
    return m_bvh->intersect(ray, m_mesh.get(), m_stride_in_32floats);
}

uint32_t Model::material_idx(IntersectionParams& intersect) const
{
    return m_mesh[intersect.triangle_idx + m_stride_in_32floats * 3 + 3];
}

Vector3<float> Model::normal(uint32_t triangle_idx) const
{
    Vector3<float> result = *(Vector3<float>*)&m_mesh[triangle_idx + 3];
    return result;
}

Model::~Model()
{
    for (int i = 0; i < m_materials.size(); ++i)
    {
        delete m_materials[i];
    }

    for (int i = 0; i < m_textures.size(); ++i)
    {
        delete m_textures[i];
    }
}

}