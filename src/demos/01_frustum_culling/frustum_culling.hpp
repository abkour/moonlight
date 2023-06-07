#pragma once 
#include "../common/scene.hpp"
#include "../common/shader.hpp"
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../simple_math.hpp"
#include "../../core/command_queue.hpp"
#include "../../core/descriptor_heap.hpp"
#include "../../core/dx12_resource.hpp"
#include "../../core/render_texture.hpp"
#include "../../core/swap_chain.hpp"
#include "../../collision/aabb.hpp"
#include "../../collision/primitive_tests.hpp"
#include "../../utility/arena_allocator.hpp"
#include "../../utility/glyph_renderer.hpp"
#include "../../../ext/DirectXTK12/Inc/DescriptorHeap.h"
#include "../../../ext/DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteFont.h"

namespace moonlight
{

class FrustumCulling : public IApplication
{

public:

    FrustumCulling(HINSTANCE);
    ~FrustumCulling();

    void flush() override;
    void on_mouse_move(LPARAM) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();
    void load_quad_shader_assets();

private:

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scene_pso;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_quad_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quad_pso;

    std::unique_ptr<DescriptorHeap> m_quad_rtv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_vs_srv_descriptor_heap;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    std::unique_ptr<DX12Resource>   m_quad_vertex_buffer;
    std::unique_ptr<DX12Resource>   m_instance_id_buffer;
    std::unique_ptr<DX12Resource>   m_instance_data_buffer;

    D3D12_VERTEX_BUFFER_VIEW m_vertex_buffer_view;
    D3D12_VERTEX_BUFFER_VIEW m_quad_vertex_buffer_view;

    std::unique_ptr<RenderTexture> m_scene_texture;
    std::vector<AABB256> m_aabbs;

    // The buffer has to be 256-byte aligned to satisfy D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
    std::size_t m_num_instances;
    std::size_t m_num_visible_instances;
    std::unique_ptr<InstanceAttributes[]> m_instance_vertex_offsets;
    std::unique_ptr<UINT[]> m_instance_ids;

    float m_elapsed_time = 0.f;

private:

    // Font related
    std::unique_ptr<GlyphRenderer> m_glyph_renderer;
    DirectX::XMFLOAT2 m_font_pos;
    std::wstring m_text_output;
};

}