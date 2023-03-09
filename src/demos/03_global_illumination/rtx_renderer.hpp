#pragma once
#include "ray_camera.hpp"
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
#include "../../core/texture2D.hpp"
#include "../../collision/aabb.hpp"
#include "../../collision/primitive_tests.hpp"
#include "../../utility/arena_allocator.hpp"
#include "../../utility/bvh.hpp"
#include "../../utility/glyph_renderer.hpp"
#include "../../../ext/DirectXTK12/Inc/DescriptorHeap.h"
#include "../../../ext/DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteBatch.h"
#include "../../../ext/DirectXTK12/Inc/SpriteFont.h"

namespace moonlight
{

class RTX_Renderer : public IApplication
{

/*
*   Requires:
*       - BVH           [x]
*       - Camera        [x]
*       - PBR           [ ]
*       - File Loading  [X]
*
*   Approach:
*       Get Camera/PBR done first, then worry about file loading, then worry about BVH
*/

public:

    RTX_Renderer(HINSTANCE);
    ~RTX_Renderer();

    bool is_application_initialized() override;
    bool is_gui_enabled() override;

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

    void parse_files(const char* filename);
    void construct_bvh();
    void generate_image();

private:

    bool app_initialized;

    Microsoft::WRL::ComPtr<ID3D12Device2>             device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       scene_pso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       quad_pso;

    std::unique_ptr<SwapChain>      swap_chain;
    std::unique_ptr<CommandQueue>   command_queue;
    std::unique_ptr<DX12Resource>   vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        vertex_buffer_view;

    D3D12_VIEWPORT viewport;
    D3D12_RECT     scissor_rect;

    float elapsed_time = 0.f;
    KeyState keyboard_state;

    std::unique_ptr<RayCamera> ray_camera;
    std::unique_ptr<DescriptorHeap> srv_descriptor_heap;
    std::unique_ptr<Texture2D> scene_texture;

private:

    struct u8_four;

    // BVH related
    std::vector<u8_four> image;
    BVH m_bvh;
    uint64_t m_num_triangles = 0;
    uint64_t m_stride_in_32floats = 0;
    std::unique_ptr<float[]> m_mesh;

    Vector2<uint32_t> old_window_dimensions;
};

}