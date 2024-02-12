#pragma once
#include "../common/scene.hpp"
#include "../common/shader.hpp"
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../simple_math.hpp"
#include "../../core/command_queue.hpp"
#include "../../core/descriptor_heap.hpp"
#include "../../core/dx12_resource.hpp"
#include "../../core/pso.hpp"
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

class PBRDemo : public IApplication
{

public:

    PBRDemo(HINSTANCE);
    ~PBRDemo();

    void flush() override;
    void on_key_event(const PackedKeyArguments key_state) override;
    void on_mouse_move() override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_gui_commands(ID3D12GraphicsCommandList* command_list);
    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();

private:

    std::unique_ptr<PipelineStateObject> m_pso_wrapper;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper_cube;

    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;

    DirectX::XMMATRIX m_cube_mvp;

    float m_elapsed_time = 0.f;
};

}