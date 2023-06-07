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

#include "../../core/pso.hpp"

namespace moonlight
{

class ShadowMapDemo : public IApplication
{

public:

    ShadowMapDemo(HINSTANCE);
    ~ShadowMapDemo();

    void render() override;
    void resize() override;
    void update() override;

private:

    void initialize_d3d12_state();
    void initialize_lightshader_objects();
    void initialize_shadowshader_objects();

    void light_pass(ID3D12GraphicsCommandList* command_list);
    void shadow_pass(ID3D12GraphicsCommandList* command_list);

private:

    std::unique_ptr<DescriptorHeap> m_lp_dsv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_rtv_descriptor_heap;
    std::unique_ptr<RenderTexture>  m_shadow_texture;

    VertexBufferResource m_vertex_buffer;

    std::unique_ptr<RasterModel> m_model;
    DirectX::XMMATRIX m_light_mvp_matrix;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_lp_depth_buffer;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_lp_root_signature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_sp_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_lightpass_pso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_shadowpass_pso;
    std::unique_ptr<RuntimeFXCCompiler> m_lightpass_ps;
    std::unique_ptr<RuntimeFXCCompiler> m_lightpass_vs;
    std::unique_ptr<RuntimeFXCCompiler> m_shadowpass_ps;
    std::unique_ptr<RuntimeFXCCompiler> m_shadowpass_vs;

    D3D12_VIEWPORT m_light_viewport;

    float m_elapsed_time = 0.f;
    float m_total_time = 0.f;

    std::unique_ptr<PipelineStateObject> m_light_pso_wrapper;
    std::unique_ptr<PipelineStateObject> m_shadow_pso_wrapper;
};

}