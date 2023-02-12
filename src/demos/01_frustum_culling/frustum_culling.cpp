#include "frustum_culling.hpp"

#include <chrono>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace moonlight {

template<typename T>
T* temp_address(T&& rval)
{
    return &rval;
}

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
} scene_pipeline_state_stream;

static struct QuadPipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
} quad_pipeline_state_stream;

struct VertexFormat
{
    XMFLOAT3 p; 
    XMFLOAT2 t;
};

static float vertices[] = {
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

static float quad_vertices[] =
{
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, -1.f, 0.f,        1.f, 0.f,
    1.f, 1.f, 0.f,        1.f, 1.f,
    -1.f, -1.f, 0.f,    0.f, 0.f,
    1.f, 1.f, 0.f,        1.f, 1.f,
    -1.f, 1.f, 0.f,        0.f, 1.f
};

struct InstanceDataFormat
{
    XMFLOAT4 displacement;
    XMFLOAT4 color;
};

// The buffer has to be 256-byte aligned to satisfy D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
static InstanceDataFormat instance_vertex_offsets[32];
static UINT instance_ids[_countof(instance_vertex_offsets)];

FrustumCulling::FrustumCulling(HINSTANCE hinstance)
    : IApplication(hinstance)
    , app_initialized(false)
    , fence_value(0)
    , camera(XMFLOAT3(0.f, 0.f, -10.f), XMFLOAT3(0.f, 0.f, 1.f))
    , top_down_camera(XMFLOAT3(0.f, -125.f, -10.f), XMFLOAT3(0.01f, 0.99f, 0.01f))
    , window_width(1600)
    , window_height(800)
{
    APressed = DPressed = SPressed = WPressed = false;
    xcoord_old = static_cast<uint32_t>(window_width) / 2;
    ycoord_old = static_cast<uint32_t>(window_height) / 2;

    window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_01_Frustum_Culling",
        window_width,
        window_height,
        &FrustumCulling::WindowMessagingProcess,
        this
    );

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device                      = _pimpl_create_device(most_sutiable_adapter);
    command_queue               = _pimpl_create_command_queue(device);
    swap_chain                  = _pimpl_create_swap_chain(command_queue, window_width, window_height);
    rtv_descriptor_heap         = _pimpl_create_rtv_descriptor_heap(device, 3);
    quad_rtv_descriptor_heap    = _pimpl_create_rtv_descriptor_heap(device, 2);
    srv_descriptor_heap         = _pimpl_create_srv_descriptor_heap(device, 2);
    dsv_descriptor_heap         = _pimpl_create_dsv_descriptor_heap(device, 2);
    instance_descriptor_heap    = _pimpl_create_srv_descriptor_heap(device, 1);
    _pimpl_create_backbuffers(device, swap_chain, rtv_descriptor_heap, backbuffers, 3);
    command_allocator           = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct         = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    fence                       = _pimpl_create_fence(device);
    fence_event                 = _pimpl_create_fence_event();

    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport0 = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window_width),
        static_cast<float>(window_height)
    );
    viewport1 = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window_width) / 2.f,
        static_cast<float>(window_height)
    );
    viewport2 = CD3DX12_VIEWPORT(
        window_width / 2, 
        0.f, 
        static_cast<float>(window_width) / 2.f, 
        static_cast<float>(window_height)
    );

    scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    scene_texture->set_device(
        device.Get(),
        quad_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), 
        srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart()
    );
    scene_texture->set_clear_color(DirectX::XMFLOAT4(0.05f, 0.05f, 0.05f, 0.f));
    scene_texture->init(window_width, window_height);

    // For orthographic view texture
    auto rtv_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto srv_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE ortho_rtv_handle(quad_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE ortho_srv_handle(srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    ortho_rtv_handle.Offset(rtv_size);
    ortho_srv_handle.Offset(srv_size);

    ortho_scene_texture = std::make_unique<RenderTexture>(DXGI_FORMAT_R8G8B8A8_UNORM);
    ortho_scene_texture->set_device(
        device.Get(),
        ortho_rtv_handle,
        ortho_srv_handle
    );
    ortho_scene_texture->set_clear_color(DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f));
    ortho_scene_texture->init(window_width, window_height);

    instance_buffer = std::make_unique<DX12Resource>();

    load_assets();
    UINT dsv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    depth_buffer = _pimpl_create_dsv(device, dsv_handle, window_width, window_height);
    dsv_handle.Offset(dsv_inc_size);
    depth_buffer2 = _pimpl_create_dsv(device, dsv_handle, window_width, window_height);

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* const command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->ExecuteCommandLists(1, command_lists);
    command_queue_signal(++fence_value);
    wait_for_fence(fence_value);

    app_initialized = true;
}

bool FrustumCulling::is_application_initialized()
{
    return app_initialized;
}

void FrustumCulling::flush()
{
    flush_command_queue();
}

void FrustumCulling::on_key_down(WPARAM wparam)
{
    switch (wparam)
    {
    case 'A':
        APressed = true;
        break;
    case 'D':
        DPressed = true;
        break;
    case 'S':
        SPressed = true;
        break;
    case 'W':
        WPressed = true;
        break;
    default:
        break;
    }
}

void FrustumCulling::on_key_up(WPARAM wparam)
{
    switch (wparam)
    {
    case 'A':
        APressed = false;
        break;
    case 'D':
        DPressed = false;
        break;
    case 'S':
        SPressed = false;
        break;
    case 'W':
        WPressed = false;
        break;
    default:
        break;
    }
}

void FrustumCulling::on_mouse_move(LPARAM lparam)
{
    if (first_cursor_entry)
    {
        xcoord_old = LOWORD(lparam);
        ycoord_old = HIWORD(lparam);
        first_cursor_entry = false;
        return;
    }

    int32_t xdelta = xcoord_old - LOWORD(lparam);
    int32_t ydelta = HIWORD(lparam) - ycoord_old;
    xcoord_old = LOWORD(lparam);
    ycoord_old = HIWORD(lparam);

    camera.rotate(xdelta, ydelta);
}

void FrustumCulling::render()
{
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
    auto backbuffer = backbuffers[backbuffer_idx];

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle2(dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    UINT dsv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    dsv_handle2.Offset(dsv_inc_size);

    CD3DX12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
    UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Clear
    {
        transition_resource(
            command_list_direct,
            backbuffer,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.1f, 0.1f, 0.1f, 1.f };
        command_list_direct->ClearRenderTargetView(
            backbuffer_rtv_handle.Offset(rtv_inc_size * backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        command_list_direct->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
        command_list_direct->ClearDepthStencilView(dsv_handle2, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
    }

    // Transition SRV to RTV
    scene_texture->transition_to_write_state(command_list_direct.Get());
    auto rt_descriptor = scene_texture->get_rtv_descriptor();
    scene_texture->clear(command_list_direct.Get());
    //
    // Record Scene
    command_list_direct->SetPipelineState(scene_pso.Get());
    command_list_direct->SetGraphicsRootSignature(scene_root_signature.Get());
    
    ID3D12Resource* instance_resource = instance_buffer->get_underlying();
    command_list_direct->SetGraphicsRootConstantBufferView(1, instance_resource->GetGPUVirtualAddress());
    command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vb_views[] =
    {
        vertex_buffer_view,
        instance_id_buffer_view
    };
    command_list_direct->IASetVertexBuffers(0, 2, vb_views);
    command_list_direct->RSSetViewports(1, &viewport0);
    command_list_direct->RSSetScissorRects(1, &scissor_rect);
    command_list_direct->OMSetRenderTargets(1, &rt_descriptor, FALSE, &dsv_handle);
    command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
    command_list_direct->DrawInstanced(sizeof(vertices) / sizeof(VertexFormat), _countof(instance_ids), 0, 0);
    // Transition RTV to SRV
    scene_texture->transition_to_read_state(command_list_direct.Get());

    //
    // Record scene again, but with top-down projection
    ortho_scene_texture->transition_to_write_state(command_list_direct.Get());
    auto ortho_rt_descriptor = ortho_scene_texture->get_rtv_descriptor();
    ortho_scene_texture->clear(command_list_direct.Get());
    command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix_v2, 0);
    dsv_handle.Offset(dsv_inc_size);
    command_list_direct->OMSetRenderTargets(1, &ortho_rt_descriptor, FALSE, &dsv_handle);
    command_list_direct->DrawInstanced((sizeof(vertices) / sizeof(VertexFormat)), _countof(instance_ids), 0, 0);
    // Transition RTV to SRV
    ortho_scene_texture->transition_to_read_state(command_list_direct.Get());

    //
    // Render the scene texture to the first viewport
    command_list_direct->SetPipelineState(quad_pso.Get());
    command_list_direct->SetGraphicsRootSignature(quad_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap.Get() };
    command_list_direct->SetDescriptorHeaps(1, heaps);
    //srv_gpu_handle.Offset();
    command_list_direct->SetGraphicsRootDescriptorTable(
        0,
        srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart()
    );

    command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list_direct->IASetVertexBuffers(0, 1, &quad_vertex_buffer_view);
    command_list_direct->RSSetViewports(1, &viewport1);
    command_list_direct->RSSetScissorRects(1, &scissor_rect);
    command_list_direct->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, NULL);
    command_list_direct->DrawInstanced(sizeof(quad_vertices) / sizeof(VertexFormat), 1, 0, 0);

    //
    // Render the ortho texture to the 2nd viewport
    
    UINT srv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle(srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart());
    srv_gpu_handle.Offset(srv_inc_size);
    command_list_direct->SetGraphicsRootDescriptorTable(
        0,
        srv_gpu_handle
    );
    command_list_direct->RSSetViewports(1, &viewport2);
    command_list_direct->DrawInstanced(sizeof(quad_vertices) / sizeof(VertexFormat), 1, 0, 0);
    
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

void FrustumCulling::resize()
{
}

void FrustumCulling::update()
{
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;

    float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    const float scale_factor = 1.f;
    XMMATRIX model_matrix = XMMatrixScaling(scale_factor, scale_factor, scale_factor);
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        100.f
    );

    uint32_t keycode = 0;
    keycode += (uint32_t)WPressed << 3;
    keycode += (uint32_t)SPressed << 2;
    keycode += (uint32_t)DPressed << 1;
    keycode += (uint32_t)APressed;
    auto pre_pos = camera.get_position();
    camera.translate(keycode, elapsed_time);
    auto post_pos = camera.get_position();
    XMFLOAT3 delta_pos(
        post_pos.x - pre_pos.x, 0.f, post_pos.z - pre_pos.z
    );
    XMVECTOR delta_pos_xmv = XMLoadFloat3(&delta_pos);
    top_down_camera.translate(delta_pos_xmv);

    mvp_matrix = XMMatrixMultiply(model_matrix, camera.view);
    mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);

    XMMATRIX projection_matrix_2 = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        200.f
    );

    const float scale_factor_2 = 1.5f;
    model_matrix = XMMatrixScaling(scale_factor_2, scale_factor_2, scale_factor_2);
    mvp_matrix_v2 = XMMatrixMultiply(model_matrix, top_down_camera.view);
    mvp_matrix_v2 = XMMatrixMultiply(mvp_matrix_v2, projection_matrix_2);

    // Ensure that the GPU has finished work, before we render the next frame.
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    // Construct the viewing frustum
    XMFLOAT3 camera_position = camera.get_position();
    XMFLOAT3 camera_direction = camera.get_direction();
    
    Frustum frustum = construct_frustum(
        *reinterpret_cast<Vector3*>(&camera_position),
        *reinterpret_cast<Vector3*>(&camera_direction),
        XMConvertToRadians(45.f),
        aspect_ratio,
        0.1,
        100.f
    );
    // Change the color of the box to yellow, if it doesn't intersect the frustum
    for (int i = 0; i < _countof(instance_ids); ++i)
    {
        constexpr int stride = 8;
        if (frustum_contains_aabb(frustum, aabbs[i]))
        {
            instance_vertex_offsets[i].color.x = 0.f;
        }
        else
        {
            instance_vertex_offsets[i].color.x = 1.f;
        }
    }
    instance_buffer->update(
        device.Get(), 
        command_list_direct.Get(), 
        instance_vertex_offsets, 
        sizeof(instance_vertex_offsets)
    );

    command_list_direct->Close();

    // Execute eall command now
    ID3D12CommandList* const command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->ExecuteCommandLists(1, command_lists);
    command_queue_signal(++fence_value);
    wait_for_fence(fence_value);
}

void FrustumCulling::load_assets()
{
    // Create a circle of boxes around the origin
    for (int i = 0; i < _countof(instance_vertex_offsets); ++i)
    {
        instance_ids[i] = i;

        static float angle = 0.f;
        static float radial_distance = 30.f;
        static float angle_increment = 360.f / _countof(instance_vertex_offsets);
        instance_vertex_offsets[i].displacement = XMFLOAT4(
            cos(angle) * radial_distance, 
            0.f, 
            sin(angle) * radial_distance, 
            0.f
        );
        angle += angle_increment;

        instance_vertex_offsets[i].color = XMFLOAT4(0.f, 1.f, 0.f, 1.f);
    }

    load_scene_shader_assets();
    load_quad_shader_assets();
    construct_aabbs();
}

void FrustumCulling::load_scene_shader_assets()
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
        vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    // Instancing data
    {
        instance_buffer->upload(
            device.Get(), 
            command_list_direct.Get(), 
            (void*)instance_vertex_offsets, 
            sizeof(instance_vertex_offsets)
        );

        // Create the CBV view
        ID3D12Resource* resource = instance_buffer->get_underlying();
        cbv_desc.BufferLocation = resource->GetGPUVirtualAddress();
        cbv_desc.SizeInBytes = sizeof(instance_vertex_offsets);
        device->CreateConstantBufferView(&cbv_desc, instance_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

        // Instance vertex buffer
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(instance_ids))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&instance_id_buffer)
        ));

        UINT32* vertex_data_begin = nullptr;
        CD3DX12_RANGE read_range(0, 0);
        ThrowIfFailed(instance_id_buffer->Map(
            0,
            &read_range,
            reinterpret_cast<void**>(&vertex_data_begin)
        ));
        memcpy(vertex_data_begin, instance_ids, sizeof(instance_ids));
        instance_id_buffer->Unmap(0, nullptr);

        instance_id_buffer_view.BufferLocation = instance_id_buffer->GetGPUVirtualAddress();
        instance_id_buffer_view.SizeInBytes = sizeof(instance_ids);
        instance_id_buffer_view.StrideInBytes = sizeof(UINT);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//instanced_cube_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 1, 0,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1
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
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsConstantBufferView(1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(2, root_parameters, 0, nullptr, root_signature_flags);

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

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&scene_pso)));
}

void FrustumCulling::load_quad_shader_assets()
{
    // Quad vertex buffer
    {
        ThrowIfFailed(device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(quad_vertices))),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&quad_vertex_buffer)
        ));

        UINT32* vertex_data_begin = nullptr;
        CD3DX12_RANGE read_range(0, 0);
        ThrowIfFailed(quad_vertex_buffer->Map(
            0,
            &read_range,
            reinterpret_cast<void**>(&vertex_data_begin)
        ));
        memcpy(vertex_data_begin, quad_vertices, sizeof(quad_vertices));
        quad_vertex_buffer->Unmap(0, nullptr);

        quad_vertex_buffer_view.BufferLocation = quad_vertex_buffer->GetGPUVirtualAddress();
        quad_vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        quad_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//fullquad_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//01_frustum_culling//shaders//fullquad_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
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

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init_1_1(1, root_parameters, 1, samplers, root_signature_flags);

    ComPtr<ID3DBlob> root_signature_blob;
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
        IID_PPV_ARGS(&quad_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    quad_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    quad_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    quad_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    quad_pipeline_state_stream.root_signature = quad_root_signature.Get();
    quad_pipeline_state_stream.rs = rasterizer_desc;
    quad_pipeline_state_stream.rtv_formats = rtv_formats;
    quad_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(QuadPipelineStateStream),
        &quad_pipeline_state_stream
    };

    ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&quad_pso)));
}

void FrustumCulling::construct_aabbs()
{
    const uint32_t n_instances = _countof(instance_ids);
    const uint32_t n_vertices = sizeof(vertices) / sizeof(VertexFormat);
    
    // Prepare the data first, magical/phantastical numbers! (do you write that word with ph or f, not sure, probably f)
    float intermediate_buffer[n_vertices * 3];
    for (int i = 0; i < n_vertices; ++i)
    {
        intermediate_buffer[i * 3] = vertices[i * 5];
        intermediate_buffer[i * 3 + 1] = vertices[i * 5 + 1];
        intermediate_buffer[i * 3 + 2] = vertices[i * 5 + 2];
    }
    
    AABB os_aabb = construct_aabb_from_points(
        reinterpret_cast<Vector3*>(intermediate_buffer),
        n_vertices
    );

    for (int j = 0; j < n_instances; ++j)
    {
        AABB new_aabb;
        new_aabb.bmin = os_aabb.bmin;
        new_aabb.bmax = os_aabb.bmax;
        new_aabb.bmin += *(Vector3*)&instance_vertex_offsets[j].displacement;
        new_aabb.bmax += *(Vector3*)&instance_vertex_offsets[j].displacement;
        aabbs.emplace_back(new_aabb);
#ifdef _DEBUG
        char buffer[512];
        sprintf_s(buffer, "bmin: (%f, %f, %f)\t\tbmax: (%f, %f, %f)\n",
            aabbs.back().bmin.x,
            aabbs.back().bmin.y,
            aabbs.back().bmin.z,
            aabbs.back().bmax.x,
            aabbs.back().bmax.y,
            aabbs.back().bmax.z
        );
        OutputDebugStringA(buffer);
#endif
    }
}

void FrustumCulling::transition_resource(
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

void FrustumCulling::command_queue_signal(uint64_t fence_value)
{
    ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value));
}

void FrustumCulling::flush_command_queue()
{
    ++fence_value;
    command_queue_signal(fence_value);
    wait_for_fence(fence_value);
}

void FrustumCulling::wait_for_fence(uint64_t fence_value)
{
    if (fence->GetCompletedValue() < fence_value)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
        WaitForSingleObject(fence_event, INFINITE);
    }
}

}