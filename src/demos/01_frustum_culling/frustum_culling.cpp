#include "frustum_culling.hpp"

#include <chrono>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace moonlight {

struct VertexFormat
{
    XMFLOAT3 p; 
    XMFLOAT2 t;
};

static float interleaved_cube_vertices[] = {
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

static float interleaved_quad_vertices[] =
{
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, -1.f, 0.f,     1.f, 0.f,
    1.f, 1.f, 0.f,      1.f, 1.f,
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, 1.f, 0.f,      1.f, 1.f,
    -1.f, 1.f, 0.f,     0.f, 1.f
};

FrustumCulling::FrustumCulling(HINSTANCE hinstance)
    : IApplication(hinstance, &FrustumCulling::WindowMessagingProcess)
{
    m_text_output.resize(256);

    m_quad_rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2
    );
    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2
    );
    m_vs_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2
    );

    const float clear_color[] = { 0.1f, 0.1f, 0.1f, 0.f };
    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    memcpy(clear_value.Color, clear_color, sizeof(clear_color));

    m_scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    m_scene_texture->set_device(
        m_device.Get(),
        m_quad_rtv_descriptor_heap->cpu_handle(), 
        m_srv_descriptor_heap->cpu_handle()
    );
    m_scene_texture->set_clear_value(clear_value);
    m_scene_texture->init(m_window->width(), m_window->height());

    //m_camera = Camera(XMFLOAT3(0.f, 0.f, -3.f), XMFLOAT3(0.f, 0.f, 0.f), 50.f);

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

FrustumCulling::~FrustumCulling()
{
}

void FrustumCulling::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    UINT rtv_inc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };

    // Transition SRV to RTV
    m_scene_texture->transition_to_write_state(command_list);
    auto rt_descriptor = m_scene_texture->get_rtv_descriptor();
    m_scene_texture->clear(command_list);
    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_scene_pso->pso());
    command_list->SetGraphicsRootSignature(m_scene_pso->root_signature());
    command_list->SetGraphicsRootShaderResourceView(1, m_instance_id_buffer->gpu_virtual_address());
    command_list->SetGraphicsRootShaderResourceView(2, m_instance_data_buffer->gpu_virtual_address());
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &rt_descriptor, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &m_mvp_matrix, 0);
    command_list->DrawInstanced(
        sizeof(interleaved_cube_vertices) / sizeof(VertexFormat), 
        m_num_visible_instances, 
        0, 0
    );
    // Transition RTV to SRV
    m_scene_texture->transition_to_read_state(command_list);

    //
    // Render the scene texture to the first viewport
    command_list->SetPipelineState(m_quad_pso->pso());
    command_list->SetGraphicsRootSignature(m_quad_pso->root_signature());

    command_list->SetDescriptorHeaps(1, heaps);
    command_list->SetGraphicsRootDescriptorTable(
        0, m_srv_descriptor_heap->gpu_handle()
    );
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->IASetVertexBuffers(0, 1, &m_quad_vertex_buffer_view);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, NULL);
    command_list->DrawInstanced(
        sizeof(interleaved_quad_vertices) / sizeof(VertexFormat), 
        1, 0, 0
    );

    m_glyph_renderer->render_text(command_list, m_command_queue->get_underlying(), m_text_output.c_str(), m_font_pos);
}

void FrustumCulling::render()
{
    IApplication::clear_rtv_dsv();

    record_command_list(m_command_list_direct.Get());

    IApplication::present();
}

void FrustumCulling::resize()
{
}

void FrustumCulling::update()
{
    static uint32_t update_count = 0;
    static float elapsed_time_at_threshold = 0.f;
    static float cull_time_at_threshold = 0.f;
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
    XMMATRIX model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    m_camera.translate(m_keyboard_state, m_elapsed_time);
    m_mvp_matrix = XMMatrixMultiply(model_matrix, m_camera.view);
    m_mvp_matrix = XMMatrixMultiply(m_mvp_matrix, projection_matrix);

    // Construct the viewing frustum
    XMFLOAT3 m_camera_position = m_camera.get_position();
    XMFLOAT3 m_camera_direction = m_camera.get_direction();
    
    Frustum frustum = construct_frustum(
        *reinterpret_cast<Vector3<float>*>(&m_camera_position),
        *reinterpret_cast<Vector3<float>*>(&m_camera_direction),
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    Frustum abs_frustum;
    std::memcpy(&abs_frustum, &frustum, sizeof(Frustum));

    {
        // Compute absolute value of each component in the frustum to initialize
        // an "abs_frustum".
        const int n_frustum_floats = sizeof(Frustum) / sizeof(float);
        float* frustum_cast = (float*)&abs_frustum;
        for (int i = 0; i < n_frustum_floats; ++i)
        {
            frustum_cast[i] = std::abs(frustum_cast[i]);
        }
    }

    auto float_set = [](float* arr, float val, int size) 
    {
        for (int i = 0; i < size; ++i)
        {
            arr[i] = val;
        }
    };
    
    FrustumSIMD frustum_avx2 = construct_frustumSIMD_from_frustum(frustum);
    FrustumSIMD abs_frustum_avx2 = construct_frustumSIMD_from_frustum(abs_frustum);

    alignas(32) float d_avx2[48];
    auto cull_t0 = std::chrono::high_resolution_clock::now();
    const Plane* planes = (Plane*)&frustum;
    for (int i = 0; i < 6; ++i)
    {
        float_set(&d_avx2[i * 8], dot(planes[i].normal, planes[i].point), 8);
    }
    
    m_num_visible_instances = 0;
    for (int i = 0; i < m_num_instances / 8; ++i)
    {
        uint8_t mask = frustum_contains_aabb_avx2(&frustum_avx2, &abs_frustum_avx2, m_aabbs[i], d_avx2);
        for (int k = 0; k < 8; ++k)
        {
            if (mask & (0x01 << k))
            {
                m_instance_ids[m_num_visible_instances] = i * 8 + k;
                ++m_num_visible_instances;
            }
        }
    }

    auto cull_t1 = std::chrono::high_resolution_clock::now();
    auto cull_time = (cull_t1 - cull_t0).count() * 1e-3;
    // Update the contents of the offset buffer
    m_instance_id_buffer->update(
        m_device.Get(), m_command_list_direct.Get(), 
        m_instance_ids.get(), sizeof(UINT) * m_num_visible_instances,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    m_text_output.resize(128);
    uint32_t n_culled_objects = m_num_instances - m_num_visible_instances;
    swprintf(
        m_text_output.data(), 
        L"Frame time: %dms\nCull time: %dus\nNumber of cubes: %d\nCulled: %d\nTriangles: %d", 
        static_cast<uint32_t>(elapsed_time_at_threshold * 1e3),
        static_cast<uint32_t>(cull_time),
        m_num_instances,
        n_culled_objects,
        n_culled_objects * 36 / 3
    );

    update_count++;
    if (update_count % 16 == 0)
    {
        cull_time_at_threshold = cull_time;
    }
    if (update_count % 25 == 0)
    {
        elapsed_time_at_threshold = m_elapsed_time;
    }
}

void FrustumCulling::load_assets()
{
    const float scene_xdim = 700.f;
    const float scene_ydim = 700.f;
    const int n_cubes_per_row = 300;
    const int n_cubes_per_column = 300;
    m_num_instances = n_cubes_per_row * n_cubes_per_column;
    
    m_instance_ids = std::make_unique<UINT[]>(m_num_instances);
    for (int i = 0; i < m_num_instances; ++i)
    {
        m_instance_ids[i] = i;
    }
    m_instance_vertex_offsets = std::make_unique<InstanceAttributes[]>(m_num_instances);
    construct_scene_of_cubes(
        m_instance_vertex_offsets.get(),
        scene_xdim, scene_ydim, 
        n_cubes_per_row, n_cubes_per_column
    );
    load_scene_shader_assets();
    load_quad_shader_assets();
    
    const uint32_t n_vertices = sizeof(interleaved_cube_vertices) / sizeof(VertexFormat);
    m_aabbs.resize(m_num_instances / 8);
    construct_instanced_aabbs(
        m_aabbs.data(), 
        m_num_instances / 8,
        interleaved_cube_vertices,
        sizeof(interleaved_cube_vertices) / sizeof(VertexFormat),
        sizeof(VertexFormat),
        reinterpret_cast<float*>(m_instance_vertex_offsets.get()),
        sizeof(InstanceAttributes)
    );
}

void FrustumCulling::load_scene_shader_assets()
{
    // Vertex buffer uploading
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(), 
            interleaved_cube_vertices, sizeof(interleaved_cube_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(interleaved_cube_vertices);
        m_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    // Instancing ID SRV
    {
        m_instance_id_buffer = std::make_unique<DX12Resource>();
        m_instance_id_buffer->upload(
            m_device.Get(), m_command_list_direct.Get(),
            m_instance_ids.get(), sizeof(UINT) * m_num_instances,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );
        
        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_num_instances;
        buffer_desc.StructureByteStride = sizeof(UINT);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            m_instance_id_buffer->get_underlying(),
            &srv_desc,
            m_vs_srv_descriptor_heap->cpu_handle()
        );
        
        m_instance_id_buffer->transition(
            m_command_list_direct.Get(), 
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    // Instance data SRV
    {
        m_instance_data_buffer = std::make_unique<DX12Resource>();
        m_instance_data_buffer->upload(
            m_device.Get(), m_command_list_direct.Get(),
            m_instance_vertex_offsets.get(), sizeof(InstanceAttributes) * m_num_instances,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_num_instances;
        buffer_desc.StructureByteStride = sizeof(InstanceAttributes);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            m_instance_data_buffer->get_underlying(),
            &srv_desc,
            m_vs_srv_descriptor_heap->cpu_handle(1)
        );

        m_instance_data_buffer->transition(
            m_command_list_direct.Get(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }

    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//01_frustum_culling//shaders//instanced_cube_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//01_frustum_culling//shaders//instanced_cube_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_t(input_layout);

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsShaderResourceView(0);
    root_parameters[2].InitAsShaderResourceView(1);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_scene_pso = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_scene_pso->construct_root_signature(
        root_parameters, _countof(root_parameters), 
        nullptr, 0
    );
    m_scene_pso->construct_rt_formats(rtv_formats);
    m_scene_pso->construct_ds_format(DXGI_FORMAT_D32_FLOAT);
    m_scene_pso->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE);
    m_scene_pso->construct_input_layout(input_layout, _countof(input_layout));
    m_scene_pso->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_scene_pso->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_scene_pso->construct();
}

void FrustumCulling::load_quad_shader_assets()
{
    // Quad vertex buffer
    {
        m_quad_vertex_buffer = std::make_unique<DX12Resource>();
        m_quad_vertex_buffer->upload(
            m_device.Get(), m_command_list_direct.Get(), 
            interleaved_quad_vertices, sizeof(interleaved_quad_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_quad_vertex_buffer_view.BufferLocation = m_quad_vertex_buffer->gpu_virtual_address();
        m_quad_vertex_buffer_view.SizeInBytes = sizeof(interleaved_quad_vertices);
        m_quad_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//01_frustum_culling//shaders//fullquad_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//01_frustum_culling//shaders//fullquad_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_t(input_layout);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_quad_pso = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_quad_pso->construct_root_signature(
        root_parameters, _countof(root_parameters), 
        samplers, _countof(samplers)
    );
    m_quad_pso->construct_input_layout(input_layout, _countof(input_layout));
    m_quad_pso->construct_rt_formats(rtv_formats);
    m_quad_pso->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_quad_pso->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_quad_pso->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_quad_pso->construct();
}

}