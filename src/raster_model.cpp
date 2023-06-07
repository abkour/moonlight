#include "raster_model.hpp"
#include <fstream>

namespace moonlight
{

RasterModel::RasterModel(const char* filename)
{
    uint64_t n_attr_data_bytes = 0;
    uint64_t n_triangles = 0;

    std::ifstream file(filename, std::ios::in | std::ios::binary);

    file.read((char*)&n_triangles, sizeof(uint64_t));
    file.read((char*)&m_stride_in_32floats, sizeof(uint64_t));

    file.read((char*)&n_attr_data_bytes, sizeof(uint64_t));
    file.read((char*)&m_model_flags, sizeof(uint64_t));

    m_num_vertices = n_triangles * 3;
    m_vertex_data = std::make_unique<float[]>(n_attr_data_bytes / sizeof(float));

    file.read((char*)m_vertex_data.get(), n_attr_data_bytes);
}

float* RasterModel::underlying_resource()
{
    return m_vertex_data.get();
}

std::size_t RasterModel::num_vertices() const
{
    return m_num_vertices;
}

std::size_t RasterModel::model_flags() const
{
    return m_model_flags;
}

std::size_t RasterModel::stride() const
{
    return m_stride_in_32floats;
}


}