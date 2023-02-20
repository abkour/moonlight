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

class EnvironmentMapping : public IApplication
{

public:

    EnvironmentMapping(HINSTANCE);
    ~EnvironmentMapping();

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
    void initialize_raw_input_devices();

private:

    bool app_initialized;

    Microsoft::WRL::ComPtr<ID3D12Device2>             device;
    Microsoft::WRL::ComPtr<ID3D12Resource>            depth_buffer;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       scene_pso;

    std::unique_ptr<SwapChain>      swap_chain;
    std::unique_ptr<CommandQueue>   command_queue;
    std::unique_ptr<DescriptorHeap> dsv_descriptor_heap;
    std::unique_ptr<DX12Resource>   vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        vertex_buffer_view;

    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor_rect;

    Camera camera;
    DirectX::XMMATRIX mvp_matrix;

    float elapsed_time = 0.f;
    KeyState keyboard_state;

private:

    // Font related
    std::unique_ptr<GlyphRenderer> glyph_renderer;
    DirectX::XMFLOAT2 font_pos;
    std::wstring text_output;
};

}