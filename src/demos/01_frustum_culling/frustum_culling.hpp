#pragma once 
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../simple_math.hpp"
#include "../../dx12_resource.hpp"
#include "../../render_texture.hpp"
#include "../../math/aabb.hpp"
#include "../../math/primitive_tests.hpp"
#include "../../utility/arena_allocator.hpp"
#include "../../../ext/DirectXTK12/Inc/DescriptorHeap.h"
#include "../../../ext/DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteFont.h"

namespace moonlight
{

class FrustumCulling : public IApplication
{

public:

    FrustumCulling(HINSTANCE hinstance);
    ~FrustumCulling();

    bool is_application_initialized() override;

    void flush() override;
    void on_key_down(WPARAM wparam) override;
    void on_key_up(WPARAM wparam) override;
    void on_mouse_move(LPARAM lparam) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    ArenaAllocator arena;

    Microsoft::WRL::ComPtr<ID3D12Device2> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> quad_rtv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> vs_srv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> backbuffers[3];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
    Microsoft::WRL::ComPtr<ID3D12CommandList> command_list_copy;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> scene_pso;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> quad_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> quad_pso;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> quad_vertex_buffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> instance_id_buffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> instance_data_buffer;
    D3D12_VERTEX_BUFFER_VIEW instance_data_buffer_view;

    uint64_t fence_value;
    HANDLE fence_event;
    D3D12_VIEWPORT viewport0;
    D3D12_RECT scissor_rect;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
    D3D12_VERTEX_BUFFER_VIEW quad_vertex_buffer_view;

    DirectX::XMMATRIX mvp_matrix;
    bool app_initialized;

    void load_assets();
    void load_scene_shader_assets();
    void load_quad_shader_assets();
    void initialize_font_rendering();
    void construct_aabbs();
    void construct_scene();

    void transition_resource(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list,
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES before_state,
        D3D12_RESOURCE_STATES after_state
    );

    void command_queue_signal(uint64_t fence_value);
    void flush_command_queue();
    void wait_for_fence(uint64_t fence_value);

    std::unique_ptr<RenderTexture> scene_texture;

    bool APressed, DPressed, SPressed, WPressed;

    //std::vector<AABB> aabbs;
    alignas(32) std::vector<AABB256> aabbs;

    Camera camera;

    struct InstanceDataFormat
    {
        DirectX::XMFLOAT4 displacement;
        DirectX::XMFLOAT4 color;
    };

    // The buffer has to be 256-byte aligned to satisfy D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
    std::size_t n_instances;
    std::size_t n_visible_instances;
    std::unique_ptr<InstanceDataFormat[]> instance_vertex_offsets;
    std::unique_ptr<UINT[]> instance_ids;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertex_intermediate_resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> instance_ids_intermediate_resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> instance_data_intermediate_resource;

private:

    // Mouse/Keyboard controls
    void initialize_raw_input_devices();
    float elapsed_time = 0.f;

private:

    // Font related
    std::unique_ptr<DirectX::SpriteBatch> sprite_batch;
    std::unique_ptr<DirectX::SpriteFont> sprite_font;
    std::unique_ptr<DirectX::DescriptorHeap> font_descriptor_heap;
    DirectX::XMFLOAT2 font_pos;
    std::wstring text_output;

    enum Descriptors
    {
        CourierFont,
        Count
    };

    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
};

}