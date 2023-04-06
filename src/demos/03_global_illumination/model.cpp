#include "model.hpp"
#include <fstream>

#include "material_lambertian.hpp"
#include "texture_single.hpp"

namespace moonlight
{

Model::Model()
{
    m_bvh = std::make_unique<BVH>();
}

void Model::build_bvh()
{
    m_bvh->build_bvh(m_mesh.get(), m_stride_in_32floats, m_num_triangles);
}

void Model::parse_mof(const std::string& filename)
{
    uint64_t n_attr_data_bytes = 0;

    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

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

        for (int i = 0; i < m_num_materials; ++i)
        {
            uint8_t material_flags = ML_MATERIAL_NONE;
            uint8_t texture_map_flags = ML_MATERIAL_MAPS_NONE;

            file.read((char*)&material_flags, sizeof(uint8_t));
            file.read((char*)&texture_map_flags, sizeof(uint8_t));

            if (material_flags & ML_MATERIAL_DIFFUSE)
            {
                Vector3<float> diffuse_color;
                file.read((char*)&diffuse_color.x, sizeof(float) * 3);

                m_textures.emplace_back(new SingleColor(
                    diffuse_color.x,
                    diffuse_color.y,
                    diffuse_color.z,
                    1.f
                ));

                m_materials[i] = new LamberrtianMaterial(m_textures.back());
            }

            //
            //
            // The rest of the loop is placeholder code, because the full mof format is
            // not yet supported by the engine.
            uint8_t plh = 0x00;
            Vector3<float> plh_color;

            if (material_flags & ML_MATERIAL_EMISSIVE)
            {
                file.read((char*)&plh_color.x, sizeof(Vector3<float>));
            }
            if (material_flags & ML_MATERIAL_SPECULAR)
            {
                file.read((char*)&plh_color.x, sizeof(Vector3<float>));
            }

            uint64_t plh_str_len = 0;
            char plh_str[512];
            if (texture_map_flags & ML_MATERIAL_MAPS_DIFFUSE)
            {
                file.read((char*)&plh_str_len, sizeof(std::size_t));
                file.read(plh_str, plh_str_len);
            }
            if (texture_map_flags & ML_MATERIAL_MAPS_EMISSIVE)
            {
                file.read((char*)&plh_str_len, sizeof(std::size_t));
                file.read(plh_str, plh_str_len);
            }
            if (texture_map_flags & ML_MATERIAL_MAPS_SPECULAR)
            {
                file.read((char*)&plh_str_len, sizeof(std::size_t));
                file.read(plh_str, plh_str_len);
            }
            if (texture_map_flags & ML_MATERIAL_MAPS_BUMP)
            {
                file.read((char*)&plh_str_len, sizeof(std::size_t));
                file.read(plh_str, plh_str_len);
            }
            if (texture_map_flags & ML_MATERIAL_MAPS_NORMAL)
            {
                file.read((char*)&plh_str_len, sizeof(std::size_t));
                file.read(plh_str, plh_str_len);
            }
        }
    }
}

Vector3<float> Model::color_rgb(const uint32_t material_idx) const
{
    Vector4<float> color = m_textures[material_idx]->color(0.f, 0.f);
    Vector3<float> res(color.x, color.y, color.z);
    return res;
}

Vector4<float> Model::color_rgba(const uint32_t material_idx) const
{
    return m_textures[material_idx]->color(0.f, 0.f);
}

IntersectionParams Model::intersect(Ray& ray) const
{
    IntersectionParams its = m_bvh->intersect(ray, m_mesh.get(), m_stride_in_32floats);
    its.point = ray.o + its.t * ray.d;
    return its;
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