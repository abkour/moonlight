#include "cube_application.hpp"

#include "../../../ext/DirectXTex/DirectXTex/DirectXTex.h"
#include <d3dcompiler.h>

#include <chrono>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace moonlight
{

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
    : IApplication(hinstance, &CubeApplication::WindowMessagingProcess)
{
    m_srv_descriptor_heap = _pimpl_create_srv_descriptor_heap(m_device, 1);

    //Load assets used for rendering
    std::wstring filename = ROOT_DIRECTORY_WIDE + std::wstring(L"//src//demos//00_cube_application//rsc//horse.png");
    load_texture_from_file(filename.c_str());
    load_assets();

    m_application_initialized = true;
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

    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        100.f
    );

    m_mvp_matrix = XMMatrixMultiply(model_matrix, view_matrix);
    m_mvp_matrix = XMMatrixMultiply(m_mvp_matrix, projection_matrix);
}

void CubeApplication::render()
{
    IApplication::clear_rtv_dsv(DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f));

    record_command_list();

    IApplication::present();
}

void CubeApplication::resize()
{
}

void CubeApplication::record_command_list()
{
    auto backbuffer_idx = m_swap_chain->current_backbuffer_index();
    auto rtv_handle = m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    auto dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap.Get() };
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer->get_view() };

    m_command_list_direct->SetPipelineState(m_pso_wrapper->pso());
    m_command_list_direct->SetGraphicsRootSignature(m_pso_wrapper->root_signature());
    m_command_list_direct->SetDescriptorHeaps(1, heaps);
    m_command_list_direct->SetGraphicsRootDescriptorTable(
        1,
        m_srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart()
    );

    m_command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_command_list_direct->IASetVertexBuffers(0, 1, vb_views);
    m_command_list_direct->RSSetViewports(1, &m_viewport);
    m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
    m_command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    m_command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &m_mvp_matrix, 0);
    m_command_list_direct->DrawInstanced(sizeof(vertices) / sizeof(VertexPosColor), 1, 0, 0);
}

void CubeApplication::load_assets()
{
    // Vertex buffer uploading
    {
        m_vertex_buffer = std::make_unique<VertexBufferResource>();
        m_vertex_buffer->upload(
            m_device.Get(),
            m_command_list_direct.Get(),
            sizeof(vertices) / sizeof(VertexPosColor),
            sizeof(VertexPosColor) / sizeof(float),
            sizeof(float), vertices
        );
    }

    std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//00_cube_application//shaders//triangle_vs.hlsl";
    std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//00_cube_application//shaders//triangle_ps.hlsl";

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

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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

    m_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper->construct_root_signature(root_parameters, _countof(root_parameters), samplers, _countof(samplers));
    m_pso_wrapper->construct_rt_formats(rtv_formats);
    m_pso_wrapper->construct_blend_desc(blend_desc);
    m_pso_wrapper->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper->construct();
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

    ThrowIfFailed(m_device->CreateCommittedResource(
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
        IID_PPV_ARGS(&m_texture)
    ));

    ThrowIfFailed(m_device->CreateCommittedResource(
        temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        temp_address(CD3DX12_RESOURCE_DESC::Buffer(scratch_image.GetPixelsSize())),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_upload_buffer)
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

    UpdateSubresources(m_command_list_direct.Get(), m_texture.Get(), m_upload_buffer.Get(), 0, 0, 1, &src_data);
    const auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_texture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    m_command_list_direct->ResourceBarrier(1, &transition);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.f;

    m_device->CreateShaderResourceView(
        m_texture.Get(),
        &srv_desc,
        m_srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart()
    );
}

}