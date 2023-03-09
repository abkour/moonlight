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

static struct ScenePipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL ds_desc;
} scene_pipeline_state_stream;

EnvironmentMapping::EnvironmentMapping(HINSTANCE hinstance)
    : IApplication(hinstance)
    , app_initialized(false)
    , camera(XMFLOAT3(0.f, 0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), 0.5f)
{
    text_output.resize(256);

    m_window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1600,
        800,
        &EnvironmentMapping::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device              = _pimpl_create_device(most_sutiable_adapter);
    command_queue       = std::make_unique<CommandQueue>(device.Get());
    command_allocator   = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    dsv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2
    );
    srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1
    );

    swap_chain = std::make_unique<SwapChain>(
        device.Get(),
        command_queue->get_underlying(),
        m_window->width(),
        m_window->height(),
        m_window->handle
    );

    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    depth_buffer = _pimpl_create_dsv(
        device,
        dsv_descriptor_heap->cpu_handle(),
        m_window->width(), m_window->height()
    );

    std::wstring filename = ROOT_DIRECTORY_WIDE + std::wstring(L"//src//demos//02_environment_mapping//rsc//skybox00/cubemap.dds");
    load_cubemap(filename.c_str());
    load_assets();

    std::wstring font_filename = std::wstring(ROOT_DIRECTORY_WIDE) + L"/rsc/myfile.spriteFont";
    glyph_renderer = std::make_unique<GlyphRenderer>(
        device.Get(),
        command_queue->get_underlying(),
        viewport,
        font_filename.c_str()
    );
    font_pos.x = static_cast<float>(m_window->width()) * 0.9;
    font_pos.y = static_cast<float>(m_window->height()) * 0.5;

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->execute_command_list(command_lists, 1);
    command_queue->signal();
    command_queue->wait_for_fence();

    app_initialized = true;
}

EnvironmentMapping::~EnvironmentMapping()
{
}

bool EnvironmentMapping::is_application_initialized()
{
    return app_initialized;
}

void EnvironmentMapping::flush()
{
    command_queue->flush();
}

void EnvironmentMapping::initialize_raw_input_devices()
{
    RAWINPUTDEVICE rids[1];
    rids[0].usUsagePage = 0x01;
    rids[0].usUsage = 0x02;
    rids[0].dwFlags = RIDEV_NOLEGACY;
    rids[0].hwndTarget = 0;

    ThrowIfFailed(RegisterRawInputDevices(rids, 1, sizeof(rids[0])));
}

void EnvironmentMapping::on_key_event(const PackedKeyArguments key_state)
{
    switch (key_state.key_state)
    {
    case PackedKeyArguments::Released:
        keyboard_state.reset(key_state.key);
        break;
    case PackedKeyArguments::Pressed:
        keyboard_state.set(key_state.key);
        break;
    }
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
        camera.rotate(-raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
    }

    delete[] lpb;
}

void EnvironmentMapping::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = dsv_descriptor_heap->cpu_handle();
    UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(scene_pso.Get());
    command_list->SetGraphicsRootSignature(scene_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap->get_underlying() };
    command_list_direct->SetDescriptorHeaps(1, heaps);
    command_list_direct->SetGraphicsRootDescriptorTable(
        1,
        srv_descriptor_heap->gpu_handle()
    );

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
    command_list->DrawInstanced(sizeof(interleaved_cube_vn) / sizeof(VertexFormat), 1, 0, 0);

    command_list->SetPipelineState(scene_pso_env.Get());
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, 
        &mvp_matrix_2, 0);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, 
        &normalized_mvp_matrix_2, sizeof(XMMATRIX) / 4);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4,
        &model_matrix, sizeof(XMMATRIX) / 2);
    command_list->SetGraphicsRoot32BitConstants(2, sizeof(XMFLOAT3) / sizeof(float), 
        temp_address(camera.get_position()), 0);
    command_list->DrawInstanced(sizeof(interleaved_cube_vn) / sizeof(VertexFormat), 1, 0, 0);

    //
    // Render the glyphs
    glyph_renderer->render_text(command_list, command_queue->get_underlying(), text_output.c_str(), font_pos);
}

void EnvironmentMapping::render()
{
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    uint8_t backbuffer_idx = swap_chain->current_backbuffer_index();

    // Clear
    {
        swap_chain->transition_to_rtv(command_list_direct.Get());

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.05f, 0.05f, 0.05f, 1.f };
        command_list_direct->ClearRenderTargetView(
            swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        command_list_direct->ClearDepthStencilView(
            dsv_descriptor_heap->cpu_handle(),
            D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL
        );
    }

    record_command_list(command_list_direct.Get());

    // Present
    {
        swap_chain->transition_to_present(command_list_direct.Get());

        command_list_direct->Close();
        ID3D12CommandList* command_lists[] =
        {
            command_list_direct.Get()
        };

        command_queue->execute_command_list(command_lists, 1);
        command_queue->signal();
        swap_chain->present();
        command_queue->wait_for_fence();
    }
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
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());
    const float scale_factor = 1.f;
    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 500.f;
    model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    camera.translate(keyboard_state, elapsed_time);
    XMFLOAT3X3 rotate_camera;
    XMStoreFloat3x3(&rotate_camera, camera.view);
    XMMATRIX rotate_camera_xmm = XMLoadFloat3x3(&rotate_camera);
    mvp_matrix = XMMatrixMultiply(model_matrix, rotate_camera_xmm);
    mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);

    model_matrix = XMMatrixScaling(scale_factor / 4.f, scale_factor / 4.f, scale_factor / 4.f);
    mvp_matrix_2 = XMMatrixMultiply(model_matrix, camera.view);
    mvp_matrix_2 = XMMatrixMultiply(mvp_matrix_2, projection_matrix);

    normalized_mvp_matrix_2 = XMMatrixTranspose(model_matrix);
    normalized_mvp_matrix_2 = XMMatrixInverse(nullptr, normalized_mvp_matrix_2);

    // Ensure that the GPU has finished work, before we render the next frame.
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->execute_command_list(command_lists, 1);
    command_queue->signal();
    command_queue->wait_for_fence();

    update_count++;
    if (update_count % 25 == 0)
    {
        elapsed_time_at_threshold = elapsed_time;
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
        vertex_buffer = std::make_unique<DX12Resource>();
        vertex_buffer->upload(device.Get(), command_list_direct.Get(),
            (float*)interleaved_cube_vn,
            sizeof(interleaved_cube_vn)
        );

        vertex_buffer_view.BufferLocation = vertex_buffer->gpu_virtual_address();
        vertex_buffer_view.SizeInBytes = sizeof(interleaved_cube_vn);
        vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/02_environment_mapping/shaders/cubemap_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/02_environment_mapping/shaders/cubemap_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    // Three float4x4
    root_parameters[0].InitAsConstants(48, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);
    root_parameters[2].InitAsConstants(sizeof(XMFLOAT3) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(
        _countof(root_parameters),  root_parameters,
        _countof(samplers),         samplers,
        root_signature_flags
    );

    ComPtr<ID3DBlob> root_signature_blob;
    // TODO: What is the error blob=
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));

    ThrowIfFailed(device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&scene_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    scene_pipeline_state_stream.ds_desc = dsv_desc;

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&scene_pso)));

    // Create shaders for environment mapping
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/02_environment_mapping/shaders/env_mapping_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/02_environment_mapping/shaders/env_mapping_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }
    
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&scene_pso_env)));
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
    
    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        &rsc_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&cube_texture)
    ));

    ID3D12Resource* d3d12_cube_texture = cube_texture.Get();

    ResourceUploadBatch upload_batch(device.Get());
    upload_batch.Begin();
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
        device.Get(),
        upload_batch,
        filename,
        &d3d12_cube_texture
    ));
    upload_batch.End(command_queue->get_underlying());

    cube_texture = d3d12_cube_texture;

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MostDetailedMip = 0;
    srv_desc.TextureCube.MipLevels = metadata.mipLevels;
    srv_desc.TextureCube.ResourceMinLODClamp = 0.f;
    srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    device->CreateShaderResourceView(
        cube_texture.Get(),
        &srv_desc,
        srv_descriptor_heap->cpu_handle()
    );

    command_list_direct->Close();
    ID3D12CommandList* command_lists[] =
    {
        command_list_direct.Get()
    };

    command_queue->execute_command_list(command_lists, 1);

    command_queue->signal();
    command_queue->wait_for_fence();

    command_allocator->Reset();
    command_list_direct->Reset(command_allocator.Get(), NULL);
}

}