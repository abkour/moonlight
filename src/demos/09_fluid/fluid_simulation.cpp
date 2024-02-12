#include "fluid_simulation.hpp"

namespace moonlight
{

const float quad_vertices[] =
{
    0.f, 0.f, 0.5f, 0.f, 0.5f, 0.5f
};

FluidSimulation::FluidSimulation(HINSTANCE hinstance)
    : IApplication(hinstance, &FluidSimulation::WindowMessagingProcess)
{
    initialize_simulation();

    m_application_initialized = true;
}

void FluidSimulation::resize()
{}

void FluidSimulation::update()
{
}

void FluidSimulation::render()
{
    IApplication::clear_rtv_dsv(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 1.f));

    record_command_list();

    IApplication::present();
}

void FluidSimulation::record_command_list()
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer.get_view() };

    m_command_list_direct->SetPipelineState(m_pso_wrapper->pso());
    m_command_list_direct->SetGraphicsRootSignature(m_pso_wrapper->root_signature());
    m_command_list_direct->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    m_command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_command_list_direct->RSSetViewports(1, &m_viewport);
    m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
    m_command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
    m_command_list_direct->DrawInstanced(3, 1, 0, 0);
}

void FluidSimulation::initialize_simulation()
{
    initialize_d3d12_objects();
}

void FluidSimulation::initialize_d3d12_objects()
{
    m_vertex_buffer.upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        _countof(quad_vertices) / 2, 2, sizeof(float), (void*)quad_vertices
    );

    std::wstring vspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/09_fluid/shaders/quad_vs.hlsl";
    std::wstring pspath
        = std::wstring(ROOT_DIRECTORY_WIDE)
        + L"/src/demos/09_fluid/shaders/quad_ps.hlsl";

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    m_pso_wrapper = std::make_unique<PipelineStateObject>(m_device, m_shared_pss_field);
    m_pso_wrapper->construct_input_layout(input_layout, _countof(input_layout));
    m_pso_wrapper->construct_root_signature(nullptr, 0, nullptr, 0);
    m_pso_wrapper->construct_rt_formats(rtv_formats);
    m_pso_wrapper->construct_vs(vspath.c_str(), L"main", L"vs_6_1");
    m_pso_wrapper->construct_ps(pspath.c_str(), L"main", L"ps_6_1");
    m_pso_wrapper->construct_rasterizer(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, TRUE);
    m_pso_wrapper->construct();
}

}