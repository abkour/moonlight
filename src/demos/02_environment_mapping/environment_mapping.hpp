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

#include "../../core/pso.hpp"

namespace moonlight
{

struct CubeTexture
{
    CubeTexture()
        : texture(nullptr)
    {}
    ~CubeTexture()
    {
        if (texture)
        {
            texture->Release();
        }
    }
    ID3D12Resource* texture;
};

class EnvironmentMapping : public IApplication
{

public:

    EnvironmentMapping(HINSTANCE);
    ~EnvironmentMapping();

    void flush() override;
    void render() override;
    void resize() override;
    void update() override;

private:

    void record_command_list(ID3D12GraphicsCommandList* command_list);
    void load_assets();
    void load_scene_shader_assets();
    void load_cubemap(const wchar_t* filename);

private:

    std::unique_ptr<PipelineStateObject> m_scene_pso;
    std::unique_ptr<PipelineStateObject> m_scene_pso_env;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_cube_texture;

    std::unique_ptr<DescriptorHeap> m_srv_descriptor_heap;
    std::unique_ptr<DX12Resource>   m_vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW        m_vertex_buffer_view;

    DirectX::XMMATRIX m_model_matrix;
    DirectX::XMMATRIX m_mvp_matrix_2;
    DirectX::XMMATRIX m_normalized_mvp_matrix_2;

    float m_elapsed_time = 0.f;

private:

    // Font related
    std::unique_ptr<GlyphRenderer> m_glyph_renderer;
    DirectX::XMFLOAT2 m_font_pos;
    std::wstring m_text_output;
};

}