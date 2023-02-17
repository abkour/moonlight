#include "cube_application.hpp"

#include "../../../ext/DirectXTex/DirectXTex/DirectXTex.h"
#include <d3dcompiler.h>

#include <chrono>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace moonlight
{

template<typename T>
T* temp_address(T&& v)
{
    return &v;
}

struct VertexPosColor {
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
};

float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

// Public implementation
CubeApplication::CubeApplication(HINSTANCE hinstance)
    : IApplication(hinstance)
{
    window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12 Demo",
        window_width,
        window_height,
        &CubeApplication::WindowMessagingProcess,
        this
    );

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device = _pimpl_create_device(most_sutiable_adapter);
    command_queue = _pimpl_create_command_queue(device);
    swap_chain = _pimpl_create_swap_chain(command_queue, window_width, window_height);
    rtv_descriptor_heap = _pimpl_create_rtv_descriptor_heap(device, 3);
    _pimpl_create_backbuffers(device, swap_chain, rtv_descriptor_heap.Get(), backbuffers, 3);
    dsv_descriptor_heap = _pimpl_create_dsv_descriptor_heap(device, 1);
    srv_descriptor_heap = _pimpl_create_srv_descriptor_heap(device, 1);
    command_allocator = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    fence = _pimpl_create_fence(device);
    fence_event = _pimpl_create_fence_event();

    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(window_width), static_cast<float>(window_height));

    //Load assets used for rendering
    std::wstring filename = ROOT_DIRECTORY_WIDE + std::wstring(L"//src//demos//00_cube_application//rsc//horse.png");
    load_texture_from_file(filename.c_str());
    load_assets();

    depth_buffer = _pimpl_create_dsv(
        device, 
        dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), 
        window_width, 
        window_height
    );

    command_list_direct->Close();

    app_initialized = true;
}

bool CubeApplication::is_application_initialized()
{
    return app_initialized;
}

void CubeApplication::update()
{
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    float elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float angle = static_cast<float>(total_time * 90.f);
    XMVECTOR rotation_axis = XMVectorSet(1, 1, 0, 0);

    const float scale_factor = 5.f;
    XMMATRIX scale_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX rotation_matrix = XMMatrixRotationAxis(rotation_axis, XMConvertToRadians(angle));
    XMMATRIX model_matrix = XMMatrixMultiply(scale_matrix, rotation_matrix);

    XMVECTOR eye_position = XMVectorSet(0, 0, -10, 1);
    XMVECTOR eye_target = XMVectorSet(0, 0, 0, 1);
    XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
    XMMATRIX view_matrix = XMMatrixLookAtLH(eye_position, eye_target, world_up);

    float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        100.f
    );

    mvp_matrix = XMMatrixMultiply(model_matrix, view_matrix);
    mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);
}

void CubeApplication::render()
{
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
    auto backbuffer = backbuffers[backbuffer_idx];

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    auto dsv_handle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

    // Clear
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        const FLOAT clear_color[] = { 0.1f, 0.1f, 0.1f, 1.f };
        command_list_direct->ClearRenderTargetView(
            rtv_handle.Offset(rtv_inc_size * backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        command_list_direct->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
    }

    // Record
    command_list_direct->SetPipelineState(pso.Get());
    command_list_direct->SetGraphicsRootSignature(root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap.Get() };
    command_list_direct->SetDescriptorHeaps(1, heaps);

    // Set slot 0 to point to srv
    command_list_direct->SetGraphicsRootDescriptorTable(
        1,
        srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart()
    );

    command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list_direct->IASetVertexBuffers(0, 1, &vertex_buffer_view);
    command_list_direct->RSSetViewports(1, &viewport);
    command_list_direct->RSSetScissorRects(1, &scissor_rect);
    command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
    command_list_direct->DrawInstanced(sizeof(vertices) / sizeof(VertexPosColor), 1, 0, 0);

    // Present
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        );

        command_list_direct->Close();
        ID3D12CommandList* const command_lists[] =
        {
            command_list_direct.Get()
        };

        command_queue->ExecuteCommandLists(1, command_lists);

        command_queue_signal(++fence_value);

        swap_chain->Present(1, 0);

        wait_for_fence(fence_value);
    }
}

void CubeApplication::resize()
{
}

void CubeApplication::flush()
{
    flush_command_queue();
}

// Private implemenatation (Triangle)
static struct PipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blend_desc;
} pipeline_state_stream;

void CubeApplication::load_assets()
{
    // Vertex buffer uploading
    {
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertex_buffer)
        ));

        UINT32* vertex_data_begin = nullptr;
        CD3DX12_RANGE read_range(0, 0);
        ThrowIfFailed(vertex_buffer->Map(
            0,
            &read_range,
            reinterpret_cast<void**>(&vertex_data_begin)
        ));
        memcpy(vertex_data_begin, vertices, sizeof(vertices));
        vertex_buffer->Unmap(0, nullptr);

        vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
        vertex_buffer_view.SizeInBytes = sizeof(vertices);
        vertex_buffer_view.StrideInBytes = sizeof(VertexPosColor);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//00_cube_application//shaders//triangle_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//00_cube_application//shaders//triangle_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

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

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };

    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(2, root_parameters, 1, samplers, root_signature_flags);

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
        IID_PPV_ARGS(&root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_BLEND_DESC blend_desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    pipeline_state_stream.root_signature = root_signature.Get();
    pipeline_state_stream.rs = rasterizer_desc;
    pipeline_state_stream.rtv_formats = rtv_formats;
    pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    pipeline_state_stream.blend_desc = blend_desc;

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(PipelineStateStream),
        &pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&pso)));
}

void CubeApplication::load_texture_from_file(
    const wchar_t* filename)
{
    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratch_image;

    ThrowIfFailed(LoadFromWICFile(
        filename,
        WIC_FLAGS_FORCE_RGB,
        &metadata,
        scratch_image
    ));

    auto alpha_mode = metadata.GetAlphaMode();

    D3D12_RESOURCE_DESC tex_desc = {};
    switch (metadata.dimension)
    {
    case TEX_DIMENSION_TEXTURE2D:
        tex_desc = CD3DX12_RESOURCE_DESC::Tex2D(
            metadata.format,
            static_cast<UINT64>(metadata.width),
            static_cast<UINT>(metadata.height),
            static_cast<UINT16>(metadata.arraySize)
        );
        break;
    default:
        throw std::invalid_argument("Bad texture sent!");
        break;
    }

    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            metadata.width,
            metadata.height,
            1,
            1)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)
    ));

    ComPtr<ID3D12Resource> upload_buffer;

    ThrowIfFailed(device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(scratch_image.GetPixelsSize())),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload_buffer)
    ));

    if (scratch_image.GetImageCount() < 1)
    {
        throw std::runtime_error("Invalid file\n");
    }

    auto image_array = scratch_image.GetImages();

    D3D12_SUBRESOURCE_DATA src_data;
    src_data.pData = image_array[0].pixels;
    src_data.RowPitch = image_array[0].rowPitch;
    src_data.SlicePitch = image_array[0].slicePitch;

    UpdateSubresources(command_list_direct.Get(), texture.Get(), upload_buffer.Get(), 0, 0, 1, &src_data);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    command_list_direct->ResourceBarrier(1, &transition);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.f;

    device->CreateShaderResourceView(
        texture.Get(),
        &srv_desc,
        srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart()
    );

    // Force transition of the texture from COPY_DEST to PIXEL_SHADER_RESOURCE before 
    // a call to the rendering function, which will reset the command list.
    command_list_direct->Close();
    ID3D12CommandList* const command_lists[] =
    {
        command_list_direct.Get()
    };

    command_queue->ExecuteCommandLists(1, command_lists);

    command_queue_signal(++fence_value);
    wait_for_fence(fence_value);

    command_allocator->Reset();
    command_list_direct->Reset(command_allocator.Get(), NULL);
}
//
// Private implementation (Modifiers)
//

void CubeApplication::command_queue_signal(uint64_t fence_value)
{
    ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value));
}

void CubeApplication::flush_command_queue()
{
    ++fence_value;
    command_queue_signal(fence_value);
    wait_for_fence(fence_value);
}

void CubeApplication::wait_for_fence(uint64_t fence_value)
{
    if (fence->GetCompletedValue() < fence_value)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
        WaitForSingleObject(fence_event, INFINITE);
    }
}

void CubeApplication::transition_resource(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES before_state,
    D3D12_RESOURCE_STATES after_state)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        before_state,
        after_state
    );

    command_list->ResourceBarrier(1, &barrier);
}

}