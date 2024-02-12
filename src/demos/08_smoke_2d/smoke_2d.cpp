#include "smoke_2d.hpp"
#include "../../utility/random_number.hpp"
#include "../../../ext/DirectXTex/DirectXTex/DirectXTex.h"

namespace moonlight
{

static Vector2<float> sample_circle(float radius)
{
    while (true)
    {
        float x = random_in_range(-radius, radius);
        float y = random_in_range(-radius, radius);
        float length = std::sqrt(x * x + y * y);
        if (length < radius)
        {
            return Vector2<float>(x, y);
        }
    }
}

const float quad[] =
{
    -1.f, -1.f, 0.f, 0.f,
    1.f, -1.f, 1.f, 0.f,
    1.f, 1.f, 1.f, 1.f,

    -1.f, -1.f, 0.f, 0.f,
    1.f, 1.f, 1.f, 1.f,
    -1.f, 1.f, 0.f, 1.f
};

Smoke2D::Smoke2D(HINSTANCE hinstance) 
    : IApplication(hinstance, &Smoke2D::WindowMessagingProcess)
{
    m_rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1
    );
    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2
    );

    initialize_d3d12_state();

    m_application_initialized = true;
}

void Smoke2D::update() 
{
    static float threshold_time = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;

    threshold_time += m_elapsed_time;

    static const float break_time = 2.f;

    float emission_rate = random_in_range_int(1, 10);

    if (m_mouse.lmb_state() == MouseInterface::MouseButtonState::Down)
    {
        // Derive the velocity of the particle from the cursor position
        static Vector2<float> emitter_position_1(-0.75f, 0.f);
        static Vector2<float> emitter_position_2(0.75f, 0.f);

        for (int i = 0; i < emission_rate; ++i)
        {
            QuadTransform new_quad;

            new_quad.m_alpha = 0.2f;
            new_quad.m_angle = radians(random_in_range(0.f, 360.f));
            new_quad.m_lifetime = random_in_range(0.f, 0.5f);
            new_quad.m_size_scale = 0.1f;
            
            // Emitter 1
            new_quad.m_position = emitter_position_1;
            new_quad.m_color = Vector3<float>(1.f, 0.f, 0.f);

            // Derive the velocity of the particle from the cursor position
            auto mouse_position = Vector2<float>(m_mouse.posx(), m_window->height() - m_mouse.posy());
            mouse_position = cwise_divide(
                mouse_position, Vector2<float>(m_window->width(), m_window->height()
            ));
            mouse_position = mouse_position * 2.f - 1.f;

            auto target_position = mouse_position + sample_circle(0.1f);
            target_position = target_position - new_quad.m_position;
            target_position = normalize(target_position);

            new_quad.m_velocity = Vector2<float>(
                target_position.x * 0.025f, target_position.y * 0.025f
            );

            m_quad_transforms.push(new_quad);
            m_num_quads = std::clamp(++m_num_quads, 0ULL, m_max_num_quads);

            //
            // Emitter 2
            new_quad.m_position = emitter_position_2;
            new_quad.m_color = Vector3<float>(1.f, 1.f, 1.f);

            target_position = mouse_position + sample_circle(0.1f);
            target_position = target_position - new_quad.m_position;
            target_position = normalize(target_position);
            new_quad.m_velocity = Vector2<float>(
                target_position.x * 0.025f, target_position.y * 0.025f
            );

            m_quad_transforms.push(new_quad);
            m_num_quads = std::clamp(++m_num_quads, 0ULL, m_max_num_quads);
        }
    }

    if (threshold_time > 0.016f)
    {
        for (int i = 0; i < m_num_quads; ++i)
        {
            m_quad_transforms[i].m_alpha = 
                std::clamp(m_quad_transforms[i].m_alpha - 0.003f, 0.05f, 1.f);
            m_quad_transforms[i].m_lifetime += threshold_time;
            m_quad_transforms[i].m_size_scale = 
                std::clamp(m_quad_transforms[i].m_size_scale + 0.0055f, 0.f, 0.35f);
            m_quad_transforms[i].m_position.x += 
                (1.f - smoothstep(m_quad_transforms[i].m_lifetime, 0.f, break_time)) 
                * m_quad_transforms[i].m_velocity.x;
            m_quad_transforms[i].m_position.y +=
                (1.f - smoothstep(m_quad_transforms[i].m_lifetime, 0.f, break_time))
                * m_quad_transforms[i].m_velocity.y;
            m_quad_transforms[i].m_angle += 0.01f;

            // Add drag
            m_quad_transforms[i].m_position.y += 0.001f;
        }

        m_srv_buffer->update(
            m_device.Get(),
            m_command_list_direct.Get(),
            m_quad_transforms.get_address(),
            sizeof(QuadTransform) * m_num_quads,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        threshold_time = 0.f;
    }
}

void Smoke2D::render() 
{
    IApplication::clear_rtv_dsv(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    render_smoke();

    IApplication::present();
}

void Smoke2D::resize() 
{
}

void Smoke2D::initialize_d3d12_state()
{
    m_num_quads = 0;
    m_max_num_quads = 2048;
    m_quad_transforms = CircularBuffer<QuadTransform>(m_max_num_quads);
    initialize_smoke_shader();
    
    std::wstring filepath = ROOT_DIRECTORY_WIDE + std::wstring(L"/src/demos/08_smoke_2d/rsc/smoke.png");
    load_smoke_texture(filepath.c_str());
}

void Smoke2D::initialize_smoke_shader()
{
    m_vertex_buffer.upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        _countof(quad) / 4, 4, sizeof(float), (void*)quad
    );

    // Instance data SRV
    {
        m_srv_buffer = std::make_unique<DX12Resource>();
        m_srv_buffer->upload(
            m_device.Get(), m_command_list_direct.Get(),
            m_quad_transforms.get_address(), sizeof(QuadTransform) * m_max_num_quads,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_max_num_quads;
        buffer_desc.StructureByteStride = sizeof(QuadTransform);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            m_srv_buffer->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle()
        );

        m_srv_buffer->transition(
            m_command_list_direct.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    std::wstring vspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/08_smoke_2d/shaders/smoke_vs.hlsl";
    std::wstring pspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/08_smoke_2d/shaders/smoke_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {   "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1 };

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsConstants(1, 0);
    root_parameters[1].InitAsShaderResourceView(0);
    root_parameters[2].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(
        0, 
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP
    );

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    D3D12_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_DEST_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
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

void Smoke2D::load_smoke_texture(const wchar_t* filename)
{
    DirectX::TexMetadata metadata = {};
    DirectX::ScratchImage scratch_image;

    ThrowIfFailed(LoadFromWICFile(
        filename,
        DirectX::WIC_FLAGS_FORCE_RGB,
        &metadata,
        scratch_image
    ));

    auto alpha_mode = metadata.GetAlphaMode();

    D3D12_RESOURCE_DESC tex_desc = {};
    switch (metadata.dimension)
    {
    case DirectX::TEX_DIMENSION_TEXTURE2D:
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

    UpdateSubresources(
        m_command_list_direct.Get(), 
        m_texture.Get(), 
        m_upload_buffer.Get(), 
        0, 0, 1, &src_data
    );
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
        m_srv_descriptor_heap->cpu_handle(1)
    );
}

void Smoke2D::render_smoke()
{
    float aspect_ratio = m_window->aspect_ratio();

    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer.get_view() };
    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };

    m_command_list_direct->SetPipelineState(m_pso_wrapper->pso());
    m_command_list_direct->SetGraphicsRootSignature(m_pso_wrapper->root_signature());
    m_command_list_direct->SetGraphicsRoot32BitConstants(
        0, 1, reinterpret_cast<void*>(&aspect_ratio), 0
    );
    m_command_list_direct->SetGraphicsRootShaderResourceView(
        1, m_srv_buffer->gpu_virtual_address()
    );
    m_command_list_direct->SetDescriptorHeaps(1, heaps);

    // Set slot 0 to point to srv
    m_command_list_direct->SetGraphicsRootDescriptorTable(
        2,
        m_srv_descriptor_heap->gpu_handle(1)
    );

    m_command_list_direct->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    m_command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_command_list_direct->RSSetViewports(1, &m_viewport);
    m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
    m_command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    m_command_list_direct->DrawInstanced(6, m_num_quads, 0, 0);
}

}