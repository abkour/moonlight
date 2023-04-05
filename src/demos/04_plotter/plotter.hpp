#pragma once
#include "../../application.hpp"
#include "../../simple_math.hpp"
#include "../../camera.hpp"
#include "../../core/command_queue.hpp"
#include "../../core/cpu_gpu_texture2d.hpp"
#include "../../core/descriptor_heap.hpp"
#include "../../core/dx12_resource.hpp"
#include "../../core/render_texture.hpp"
#include "../../core/swap_chain.hpp"

namespace moonlight
{

class Plotter : public IApplication
{
public:

    Plotter(HINSTANCE);

    bool is_application_initialized() override;

    void flush() override;
    void on_key_event(const PackedKeyArguments) override;
    void on_mouse_move(LPARAM) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void initialize_raw_input_devices();
    void load_assets();

private:

    bool m_app_initialized;

    int m_num_points = 0;

    Microsoft::WRL::ComPtr<ID3D12Device2>             m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_scene_pso;

    std::unique_ptr<SwapChain>      m_swap_chain;
    std::unique_ptr<CommandQueue>   m_command_queue;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT     m_scissor_rect;

    Camera camera;
    KeyState keyboard_state;
    DirectX::XMMATRIX mvp_matrix;

    float elapsed_time = 0.f;
};

}