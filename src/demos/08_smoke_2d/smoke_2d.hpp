#pragma once
#include "../common/scene.hpp"
#include "../common/shader.hpp"
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../raster_model.hpp"
#include "../../simple_math.hpp"
#include "../../core/command_queue.hpp"
#include "../../core/descriptor_heap.hpp"
#include "../../core/dx12_resource.hpp"
#include "../../core/d3d12_runtime_fxc.hpp"
#include "../../core/render_texture.hpp"
#include "../../core/pso.hpp"
#include "../../core/swap_chain.hpp"
#include "../../core/vertex_buffer_resource.hpp"
#include "../../collision/aabb.hpp"
#include "../../collision/primitive_tests.hpp"
#include "../../utility/arena_allocator.hpp"
#include "../../utility/glyph_renderer.hpp"
#include "../../../ext/DirectXTK12/Inc/DescriptorHeap.h"
#include "../../../ext/DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteFont.h"

#include "circular_buffer.hpp"
#include "quad_transform.hpp"

namespace moonlight
{

class Smoke2D : public IApplication
{
public:

    Smoke2D(HINSTANCE);

    void update() override;
    void render() override;
    void resize() override;

private:

    void initialize_d3d12_state();
    void initialize_smoke_shader();
    void render_smoke();
    void load_smoke_texture(const wchar_t* filename);

private:

    float m_elapsed_time = 0.f;
    float m_total_time = 0.f;

    std::unique_ptr<DescriptorHeap> m_rtv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;

    VertexBufferResource m_vertex_buffer;
    std::unique_ptr<RasterModel> m_model;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper;

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;

    std::unique_ptr<DX12Resource> m_srv_buffer;
    CircularBuffer<QuadTransform> m_quad_transforms;
    std::size_t m_num_quads;
    std::size_t m_max_num_quads;
};

}