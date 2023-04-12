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

class Tetris : public IApplication
{

public:

    Tetris(HINSTANCE);
    ~Tetris();

    void flush() override;
    void on_mouse_move(LPARAM) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_gui_commands(ID3D12GraphicsCommandList* command_list);
    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();

private:

    Microsoft::WRL::ComPtr<ID3D12Device2>             m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_depth_buffer;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_scene_pso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_cube_pso;

    std::unique_ptr<SwapChain>      m_swap_chain;
    std::unique_ptr<CommandQueue>   m_command_queue;
    std::unique_ptr<DescriptorHeap> m_dsv_descriptor_heap;
    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT     m_scissor_rect;

    DirectX::XMMATRIX m_mvp_matrix;
    DirectX::XMMATRIX m_cube_mvp;

    float m_elapsed_time = 0.f;

private:

    // Game related
    struct Playfield;
    std::unique_ptr<Playfield> m_field;
    std::vector<unsigned> instances_buffer;
    std::unique_ptr<DX12Resource> instance_buffer_rsc;

    void update_instanced_buffer();
};

}