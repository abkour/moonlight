#include "environment_mapping.hpp"
#include "../../../ext/DirectXTex/DirectXTex/DirectXTex.h"
#include "../../../ext/DirectXTK12/Inc/DDSTextureLoader.h"

#include <chrono>
#include <numeric>  // for std::iota

using namespace DirectX;
using namespace Microsoft::WRL;

namespace moonlight {

struct VertexFormat
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
};

static float interleaved_cube_vn[] = {
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

EnvironmentMapping::EnvironmentMapping(HINSTANCE hinstance)
    : IApplication(hinstance, &EnvironmentMapping::WindowMessagingProcess)
{
    m_text_output.resize(256);

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1
    );

    std::wstring filename = ROOT_DIRECTORY_WIDE + std::wstring(L"//src//demos//02_environment_mapping//rsc//skybox00/cubemap.dds");
    load_cubemap(filename.c_str());
    load_assets();

    std::wstring font_filename = std::wstring(ROOT_DIRECTORY_WIDE) + L"/rsc/myfile.spriteFont";
    m_glyph_renderer = std::make_unique<GlyphRenderer>(
        m_device.Get(),
        m_command_queue->get_underlying(),
        m_viewport,
        font_filename.c_str()
    );
    m_font_pos.x = static_cast<float>(m_window->width()) * 0.9;
    m_font_pos.y = static_cast<float>(m_window->height()) * 0.5;

    m_application_initialized = true;
}

EnvironmentMapping::~EnvironmentMapping()
{
}

void EnvironmentMapping::flush()
{
    m_command_queue->flush();
}

void EnvironmentMapping::on_mouse_move(LPARAM lparam)
{
    UINT size;

    GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[size];
    if (lpb == NULL) return;

    if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER)) != size)
    {
        OutputDebugStringA("GetRawInputData does not report correct size");
    }
    RAWINPUT* raw = (RAWINPUT*)lpb;
    if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        m_camera.rotate(-raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
    }

    delete[] lpb;
}

void EnvironmentMapping::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    UINT rtv_inc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_scene_pso->pso());
    command_list->SetGraphicsRootSignature(m_scene_pso->root_signature());

    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
    m_command_list_direct->SetDescriptorHeaps(1, heaps);
    m_command_list_direct->SetGraphicsRootDescriptorTable(
        1,
        m_srv_descriptor_heap->gpu_handle()
    );

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &m_mvp_matrix, 0);
    command_list->DrawInstanced(sizeof(interleaved_cube_vn) / sizeof(VertexFormat), 1, 0, 0);

    command_list->SetPipelineState(m_scene_pso_env->pso());
    command_list->SetGraphicsRootSignature(m_scene_pso_env->root_signature());
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, 
        &m_mvp_matrix_2, 0);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, 
        &m_normalized_mvp_matrix_2, sizeof(XMMATRIX) / 4);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4,
        &m_model_matrix, sizeof(XMMATRIX) / 2);
    command_list->SetGraphicsRoot32BitConstants(2, sizeof(XMFLOAT3) / sizeof(float), 
        temp_address(m_camera.get_position()), 0);
    command_list->DrawInstanced(sizeof(interleaved_cube_vn) / sizeof(VertexFormat), 1, 0, 0);

    //
    // Render the glyphs
    m_glyph_renderer->render_text(
        command_list, 
        m_command_queue->get_underlying(), 
        m_text_output.c_str(), 
        m_font_pos
    );
}

void EnvironmentMapping::render()
{
    IApplication::clear_rtv_dsv(XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    record_command_list(m_command_list_direct.Get());

    IApplication::present();
}

void EnvironmentMapping::resize()
{
}

void EnvironmentMapping::update()
{
    static uint32_t update_count = 0;
    static float elapsed_time_at_threshold = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += m_elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());
    const float scale_factor = 1.f;
    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 500.f;
    m_model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    m_camera.translate(m_keyboard_state, m_elapsed_time);
    XMFLOAT3X3 rotate_camera;
    XMStoreFloat3x3(&rotate_camera, m_camera.view);
    XMMATRIX rotate_camera_xmm = XMLoadFloat3x3(&rotate_camera);
    m_mvp_matrix = XMMatrixMultiply(m_model_matrix, rotate_camera_xmm);
    m_mvp_matrix = XMMatrixMultiply(m_mvp_matrix, projection_matrix);

    m_model_matrix = XMMatrixScaling(scale_factor / 4.f, scale_factor / 4.f, scale_factor / 4.f);
    m_mvp_matrix_2 = XMMatrixMultiply(m_model_matrix, m_camera.view);
    m_mvp_matrix_2 = XMMatrixMultiply(m_mvp_matrix_2, projection_matrix);

    m_normalized_mvp_matrix_2 = XMMatrixTranspose(m_model_matrix);
    m_normalized_mvp_matrix_2 = XMMatrixInverse(nullptr, m_normalized_mvp_matrix_2);

    update_count++;
    if (update_count % 25 == 0)
    {
        elapsed_time_at_threshold = m_elapsed_time;
    }
}

void EnvironmentMapping::load_assets()
{
    load_scene_shader_assets();
}

void EnvironmentMapping::load_scene_shader_assets()
{
    // Vertex buffer uploading
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            (float*)interleaved_cube_vn,
            sizeof(interleaved_cube_vn),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(interleaved_cube_vn);
        m_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/02_environment_mapping/shaders/cubemap_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/02_environment_mapping/shaders/cubemap_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    // Three float4x4
    root_parameters[0].InitAsConstants(48, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameters[2].InitAsConstants(sizeof(XMFLOAT3) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    m_scene_pso
        = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_scene_pso->construct_root_signature(
        root_parameters, _countof(root_parameters),
        samplers, _countof(samplers)
    );
    m_scene_pso->construct_ds_format(DXGI_FORMAT_D32_FLOAT);
    m_scene_pso->construct_input_layout(input_layout, _countof(input_layout));
    m_scene_pso->construct_ds_desc(dsv_desc);
    m_scene_pso->construct_rt_formats(rtv_formats);
    m_scene_pso->construct_vs(vspath.c_str(), "main", "vs_5_1");
    m_scene_pso->construct_ps(pspath.c_str(), "main", "ps_5_1");
    m_scene_pso->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_scene_pso->construct();

    //
    // 
    // Create shaders for environment mapping
    vspath = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/02_environment_mapping/shaders/env_mapping_vs.hlsl";
    pspath = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"/src/demos/02_environment_mapping/shaders/env_mapping_ps.hlsl";

    m_scene_pso_env 
        = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_scene_pso_env->construct_root_signature(
        root_parameters, _countof(root_parameters), 
        samplers, _countof(samplers)
    );
    m_scene_pso_env->construct_ds_format(DXGI_FORMAT_D32_FLOAT);
    m_scene_pso_env->construct_input_layout(input_layout, _countof(input_layout));
    m_scene_pso_env->construct_ds_desc(dsv_desc);
    m_scene_pso_env->construct_rt_formats(rtv_formats);
    m_scene_pso_env->construct_vs(vspath.c_str(), "main", "vs_5_1");
    m_scene_pso_env->construct_ps(pspath.c_str(), "main", "ps_5_1");
    m_scene_pso_env->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_scene_pso_env->construct();
}

void EnvironmentMapping::load_cubemap(const wchar_t* filename)
{
    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratch_image;
    LoadFromDDSFile(
        filename,
        DDS_FLAGS_NONE,
        &metadata,
        scratch_image
    );

    D3D12_RESOURCE_DESC rsc_desc = {};
    rsc_desc.DepthOrArraySize = metadata.arraySize;
    rsc_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rsc_desc.Format = metadata.format;
    rsc_desc.MipLevels = metadata.mipLevels;
    rsc_desc.Width = metadata.width;
    rsc_desc.Height = metadata.height;
    rsc_desc.Flags = (D3D12_RESOURCE_FLAGS)metadata.miscFlags;
    rsc_desc.SampleDesc = { 1, 0 };
    
    ThrowIfFailed(m_device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &rsc_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_cube_texture)
    ));

    ID3D12Resource* d3d12_cube_texture = m_cube_texture.Get();

    ResourceUploadBatch upload_batch(m_device.Get());
    upload_batch.Begin();
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
        m_device.Get(),
        upload_batch,
        filename,
        &d3d12_cube_texture
    ));
    upload_batch.End(m_command_queue->get_underlying());

    m_cube_texture = d3d12_cube_texture;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MostDetailedMip = 0;
    srv_desc.TextureCube.MipLevels = metadata.mipLevels;
    srv_desc.TextureCube.ResourceMinLODClamp = 0.f;
    srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    m_device->CreateShaderResourceView(
        m_cube_texture.Get(),
        &srv_desc,
        m_srv_descriptor_heap->cpu_handle()
    );
}

}