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
#include "../../math/aabb.hpp"
#include "../../math/primitive_tests.hpp"
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

    bool is_application_initialized() override;

    void flush() override;
    void on_key_event(const PackedKeyArguments) override;
    void on_mouse_move(LPARAM) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();
    void load_quad_shader_assets();
    void initialize_raw_input_devices();

private:

    bool app_initialized;

    Microsoft::WRL::ComPtr<ID3D12Device2>             device;
    Microsoft::WRL::ComPtr<ID3D12Resource>            depth_buffer;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       scene_pso;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       quad_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       quad_pso;

    std::unique_ptr<SwapChain>      swap_chain;
    std::unique_ptr<CommandQueue>   command_queue;
    std::unique_ptr<DescriptorHeap> quad_rtv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> dsv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> srv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> vs_srv_descriptor_heap;
    std::unique_ptr<DX12Resource>   vertex_buffer;
    std::unique_ptr<DX12Resource>   quad_vertex_buffer;
    std::unique_ptr<DX12Resource>   instance_id_buffer;
    std::unique_ptr<DX12Resource>   instance_data_buffer;

    D3D12_VIEWPORT viewport0;
    D3D12_RECT scissor_rect;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
    D3D12_VERTEX_BUFFER_VIEW quad_vertex_buffer_view;

    Camera camera;
    DirectX::XMMATRIX mvp_matrix;
    std::unique_ptr<RenderTexture> scene_texture;
    std::vector<AABB256> aabbs;

    // The buffer has to be 256-byte aligned to satisfy D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
    std::size_t n_instances;
    std::size_t n_visible_instances;
    std::unique_ptr<InstanceAttributes[]> instance_vertex_offsets;
    std::unique_ptr<UINT[]> instance_ids;

    float elapsed_time = 0.f;
    KeyState keyboard_state;

private:

    // Font related
    std::unique_ptr<GlyphRenderer> glyph_renderer;
    DirectX::XMFLOAT2 font_pos;
    std::wstring text_output;
};

}