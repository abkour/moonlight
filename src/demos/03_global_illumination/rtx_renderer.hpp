#pragma once
#include "ray_camera.hpp"
#include "../common/scene.hpp"
#include "../common/shader.hpp"
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../simple_math.hpp"
#include "../../core/command_queue.hpp"
#include "../../core/cpu_gpu_texture2d.hpp"
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

    void flush() override;
    void on_key_event(const PackedKeyArguments) override;
    void on_mouse_move(LPARAM) override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void record_gui_commands(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();
    void initialize_raw_input_devices();

private:

    void parse_files(const char* filename);
    void construct_bvh();
    void generate_image();
    void generate_image_cs();   // compute shader
    void generate_image_mt();   // multi-threaded cpu
    void generate_image_st();   // single-threaded cpu
    void upload_to_texture();

    bool rtx_use_multithreading = false;

private:

    bool app_initialized;

    Microsoft::WRL::ComPtr<ID3D12Device2>             m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list_direct;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_scene_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_scene_pso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>       m_quad_pso;

    std::unique_ptr<SwapChain>      m_swap_chain;
    std::unique_ptr<CommandQueue>   m_command_queue;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT     m_scissor_rect;

    float m_elapsed_time = 0.f;
    KeyState m_keyboard_state;

    std::unique_ptr<RayCamera> m_ray_camera;
    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<CPUGPUTexture2D> m_texture_cpu_uploader;

private:

    struct u8_four;

    // BVH related
    std::vector<u8_four> m_image;
    BVH m_bvh;
    uint64_t m_num_triangles = 0;
    uint64_t m_stride_in_32floats = 0;
    std::unique_ptr<float[]> m_mesh;

    Vector2<uint32_t> m_old_window_dimensions;

private:

    // GUI related
    bool show_demo_window = true;
    bool show_another_window = true;

    enum TracingMethod
    {
        SingleThreaded = 0,
        MultiThreaded  = 1,
        ComputeShader  = 2
    };

    TracingMethod m_tracing_method;
    void on_resource_invalidation();
    void on_switch_tracing_method(TracingMethod prev_tracing_method);

private:

    // Compute shader related
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_cs_root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_cs_pso;

    std::unique_ptr<DX12Resource> m_uav_bvhnodes_rsc;
    std::unique_ptr<DX12Resource> m_uav_tris_rsc;
    std::unique_ptr<DX12Resource> m_uav_tris_indices_rsc;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_dst_texture;
    D3D12_RESOURCE_STATES m_dst_texture_state;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_compute_command_allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_compute_command_list;
    std::unique_ptr<CommandQueue>   m_compute_command_queue;

    void initialize_cs_pipeline();
    void dispatch_compute_shader();
};

}