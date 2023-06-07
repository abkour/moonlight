#include "shadow_map_demo.hpp"
#include "../../core/pss_desc.hpp"

using namespace Microsoft::WRL;
using namespace DirectX;

#define RENDERTEXTURE_SRV_INDEX 0
#define SRV_INDEX_COUNT 1

#define SHADOW_WIDTH 4096
#define SHADOW_HEIGHT 4096

namespace moonlight 
{

ShadowMapDemo::ShadowMapDemo(HINSTANCE hinstance)
    : IApplication(hinstance, &ShadowMapDemo::WindowMessagingProcess)
{
    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, SRV_INDEX_COUNT
    );
    m_rtv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1
    );
    m_lp_dsv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1
    );
    m_lp_depth_buffer = _pimpl_create_dsv(
        m_device,
        m_lp_dsv_descriptor_heap->cpu_handle(),
        SHADOW_WIDTH, SHADOW_HEIGHT
    );

    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = DXGI_FORMAT_R32_FLOAT;
    clear_value.Color[0] = 0.f;

    m_shadow_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R32_FLOAT);
    m_shadow_texture->set_device(
        m_device.Get(),
        m_rtv_descriptor_heap->cpu_handle(),
        m_srv_descriptor_heap->cpu_handle(RENDERTEXTURE_SRV_INDEX)
    );
    m_shadow_texture->set_clear_value(clear_value);
    m_shadow_texture->init(m_window->width(), m_window->height());

    m_light_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    initialize_d3d12_state();

    m_application_initialized = true;
}

ShadowMapDemo::~ShadowMapDemo() 
{}

void ShadowMapDemo::light_pass(ID3D12GraphicsCommandList* command_list)
{
    static const XMFLOAT3 light_position_f3(-5.f, 6.f, -1.f);
    static const XMFLOAT3 light_direction_f3(1.f, -1.f, 1.f);
    static const XMFLOAT3 world_up_f3(0.f, 1.f, 0.f);
    static auto light_position  = XMLoadFloat3(&light_position_f3);
    static auto light_direction = XMLoadFloat3(&light_direction_f3);
    static auto world_up        = XMLoadFloat3(&world_up_f3);
    static auto light_look_at   = XMMatrixLookAtLH(light_position, light_direction, world_up);
    static auto op_matrix       = XMMatrixOrthographicOffCenterLH(-10.f, 10.f, -10.f, 10.f, 1.f, 20.f);

    m_light_mvp_matrix = XMMatrixMultiply(light_look_at, op_matrix);

    m_shadow_texture->transition_to_write_state(command_list);
    m_shadow_texture->clear(command_list);
    
    auto rtv_handle = m_shadow_texture->get_rtv_descriptor();
    auto dsv_handle = m_lp_dsv_descriptor_heap->cpu_handle();
    m_command_list_direct->ClearDepthStencilView(
        dsv_handle,
        D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL
    );

    command_list->SetPipelineState(m_light_pso_wrapper->pso());
    command_list->SetGraphicsRootSignature(m_light_pso_wrapper->root_signature());

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer.get_view()};
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_light_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_light_mvp_matrix, 0);
    command_list->DrawInstanced(m_model->num_vertices(), 1, 0, 0);

    m_shadow_texture->transition_to_read_state(command_list);
}

void ShadowMapDemo::shadow_pass(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    command_list->SetPipelineState(m_shadow_pso_wrapper->pso());
    command_list->SetGraphicsRootSignature(m_shadow_pso_wrapper->root_signature());

    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
    command_list->SetDescriptorHeaps(1, heaps);

    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_mvp_matrix, 0);
    command_list->SetGraphicsRoot32BitConstants(1, sizeof(XMMATRIX) / sizeof(float), &m_light_mvp_matrix, 0);
    command_list->SetGraphicsRootDescriptorTable(
        2, m_srv_descriptor_heap->gpu_handle(RENDERTEXTURE_SRV_INDEX)
    );

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer.get_view()};
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->DrawInstanced(m_model->num_vertices(), 1, 0, 0);
}

void ShadowMapDemo::render()
{
    IApplication::clear_rtv_dsv();

    light_pass(m_command_list_direct.Get());
    shadow_pass(m_command_list_direct.Get());

    IApplication::present();
}

void ShadowMapDemo::resize()
{}

void ShadowMapDemo::update()
{
    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());

    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 500.f;
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;

    m_camera.translate(m_keyboard_state, m_elapsed_time);
    m_mvp_matrix = XMMatrixMultiply(m_camera.view, projection_matrix);

    static bool r_pressed = false;
    if (m_keyboard_state['R'] && !r_pressed)
    {
        initialize_lightshader_objects();
        initialize_shadowshader_objects();
        r_pressed = true;
    }
    if (!m_keyboard_state['R'] && r_pressed)
    {
        r_pressed = false;
    }
}

void ShadowMapDemo::initialize_d3d12_state() 
{
    std::string filepath = std::string(ROOT_DIRECTORY_ASCII) + "/src/demos/07_shadowmap/res/boxes_planes.mof";
    m_model = std::make_unique<RasterModel>(filepath.c_str());
    
    m_vertex_buffer.upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        m_model->num_vertices(),
        m_model->stride(),
        sizeof(float),
        m_model->underlying_resource()
    );

    initialize_lightshader_objects();
    initialize_shadowshader_objects();
}

void ShadowMapDemo::initialize_lightshader_objects()
{
    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//07_shadowmap//shaders//lightpass_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//07_shadowmap//shaders//lightpass_ps.hlsl";
    
    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    D3D12_RT_FORMAT_ARRAY rt_format_array = {};
    rt_format_array.NumRenderTargets = 1;
    rt_format_array.RTFormats[0] = DXGI_FORMAT_R32_FLOAT;

    m_light_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_light_pso_wrapper->construct_root_signature(root_parameters, _countof(root_parameters), nullptr, 0);
    m_light_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_light_pso_wrapper->construct_depth_buffer(DXGI_FORMAT_D32_FLOAT);
    m_light_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_light_pso_wrapper->construct_vs(vspath.c_str(), "main", "vs_5_1");
    m_light_pso_wrapper->construct_ps(pspath.c_str(), "main", "ps_5_1");
    m_light_pso_wrapper->construct_rt_formats(rt_format_array);
    m_light_pso_wrapper->construct();
}

void ShadowMapDemo::initialize_shadowshader_objects()
{
    std::wstring vspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//07_shadowmap//shaders//shadowpass_vs.hlsl";
    std::wstring pspath 
        = std::wstring(ROOT_DIRECTORY_WIDE) 
        + L"//src//demos//07_shadowmap//shaders//shadowpass_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[2];
    construct_input_layout_v_n(input_layout);

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };

    CD3DX12_ROOT_PARAMETER1 root_parameters[3];
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[2].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    
    D3D12_RT_FORMAT_ARRAY rt_format_array = {};
    rt_format_array.NumRenderTargets = 1;
    rt_format_array.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_shadow_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_shadow_pso_wrapper->construct_root_signature(
        root_parameters, _countof(root_parameters), 
        samplers, _countof(samplers)
    );
    m_shadow_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_shadow_pso_wrapper->construct_depth_buffer(DXGI_FORMAT_D32_FLOAT);
    m_shadow_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_shadow_pso_wrapper->construct_vs(vspath.c_str(), "main", "vs_5_1");
    m_shadow_pso_wrapper->construct_ps(pspath.c_str(), "main", "ps_5_1");
    m_shadow_pso_wrapper->construct_rt_formats(rt_format_array);
    m_shadow_pso_wrapper->construct();
}

}