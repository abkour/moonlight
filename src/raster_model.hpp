#pragma once
#include <memory>

namespace moonlight
{

class RasterModel
{

public:

    RasterModel(const char* filename);

    float* underlying_resource();

    std::size_t num_vertices() const;

    std::size_t stride() const;

    std::size_t model_flags() const;

private:

    std::unique_ptr<float[]> m_vertex_data;
    std::size_t m_num_vertices;
    std::size_t m_stride_in_32floats;
    std::size_t m_model_flags;
};

}