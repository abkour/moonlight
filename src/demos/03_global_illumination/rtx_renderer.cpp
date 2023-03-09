#include "rtx_renderer.hpp"
#include "../../utility/random_number.hpp"

#include <chrono>
#include <numeric>  // for std::iota

using namespace DirectX;
using namespace Microsoft::WRL;

namespace moonlight {

struct RTX_Renderer::u8_four
{
    u8_four() = default;

    u8_four(unsigned char aa, unsigned char bb, unsigned char cc, unsigned char dd)
        : r(aa), g(bb), b(cc), a(dd)
    {}

    unsigned char r, g, b, a;
};

struct VertexFormat
{
    XMFLOAT2 Position;
    XMFLOAT2 TexCoord;
};

static float quad_vertices[] =
{
    -1.f, -1.f, 0.f, 0.f,
    1.f, -1.f, 1.f, 0.f,
    1.f, 1.f, 1.f, 1.f,

    -1.f, -1.f, 0.f, 0.f,
    1.f, 1.f, 1.f, 1.f,
    -1.f, 1.f, 0.f, 1.f
};

static struct ScenePipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
} scene_pipeline_state_stream;

RTX_Renderer::RTX_Renderer(HINSTANCE hinstance)
    : IApplication(hinstance)
    , app_initialized(false)
{
    window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1280,
        960,
        &RTX_Renderer::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    device = _pimpl_create_device(most_sutiable_adapter);
    command_queue = std::make_unique<CommandQueue>(device.Get());
    command_allocator = _pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list_direct = _pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    swap_chain = std::make_unique<SwapChain>(
        device.Get(),
        command_queue->get_underlying(),
        window->width(),
        window->height(),
        window->handle
    );

    scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(window->width()),
        static_cast<float>(window->height())
    );

    srv_descriptor_heap = 
        std::make_unique<DescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2);

    ray_camera = std::make_unique<RayCamera>(
        Vector2<uint16_t>(window->width(), window->height())
    );

    ray_camera->initializeVariables(
        Vector3<float>(-3.5f, 1.f, -2.5f),
        normalize(Vector3<float>(0.f, 0.707f, 0.707)),
        45,
        1
    );

    old_window_dimensions = { window->width(), window->height() };
    image.resize(old_window_dimensions.x * old_window_dimensions.y);

    construct_bvh();
    generate_image();

    load_assets();

    command_list_direct->Close();
    // Execute eall command now
    ID3D12CommandList* command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->execute_command_list(command_lists, 1);
    command_queue->signal();
    command_queue->wait_for_fence();

    {
        // IMGUI initialization
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(window->handle);
        ImGui_ImplDX12_Init(
            device.Get(),
            3,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            srv_descriptor_heap->get_underlying(),
            srv_descriptor_heap->cpu_handle(1),
            srv_descriptor_heap->gpu_handle(1)
        );
    }

    app_initialized = true;
}

RTX_Renderer::~RTX_Renderer()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

static bool ray_hit_circle(const Ray& ray, const float r)
{
    float tz = -ray.o.z / ray.d.z;
    Vector3<float> P = ray.o + tz * ray.d;
    return length(P) <= r;
}

void RTX_Renderer::parse_files(const char* filename)
{
    uint64_t n_bytes = 0;
    uint64_t flags = 0;

    std::ifstream file(filename, std::ios::in | std::ios::binary);

    file.read((char*)&m_num_triangles, sizeof(uint64_t));
    file.read((char*)&m_stride_in_32floats, sizeof(uint64_t));
    file.read((char*)&n_bytes, sizeof(uint64_t));
    file.read((char*)&flags, sizeof(uint64_t));

    m_mesh = std::make_unique<float[]>(n_bytes / sizeof(float));

    file.read((char*)m_mesh.get(), n_bytes);
}

void RTX_Renderer::construct_bvh()
{
    std::string asset_path =
        std::string(ROOT_DIRECTORY_ASCII) + std::string("//assets//boxplane.mof");

    parse_files(asset_path.c_str());

    std::string binstr = std::string(ROOT_DIRECTORY_ASCII) + "//assets//boxplane.rawbin";
    m_bvh.build_bvh(m_mesh.get(), m_stride_in_32floats, m_num_triangles);
}

void RTX_Renderer::generate_image()
{
    const Vector3<float> center(0.f, 0.f, 0.f);
    const float radius = 0.25f;

    for (uint16_t y = 0; y < window->height(); y += 4)
    {
        for (uint16_t x = 0; x < window->width(); x += 4)
        {
            for (uint16_t v = 0; v < 4; ++v)
            {
                for (uint16_t u = 0; u < 4; ++u)
                {
                    std::size_t idx = ((y + v) * window->width()) + x + u;
                    if (idx >= image.size()) break;
                    uint16_t px = x + u;
                    uint16_t py = y + v;
                    auto ray = ray_camera->getRay({ px, py });
                    IntersectionParams intersect;
                    m_bvh.intersect(ray, m_mesh.get(), m_stride_in_32floats, intersect);
                    if (ray.t < std::numeric_limits<float>::max())
                    {
                        unsigned c = (int)(ray.t * 42);
                        c *= 0x10101;
                        image[idx].r = c;
                        image[idx].g = c;
                        image[idx].b = c;
                        image[idx].a = c;
                    } 
                    else
                    {
                        image[idx].r = 0;
                        image[idx].g = 0;
                        image[idx].b = 0;
                        image[idx].a = 0;
                    }
                }
            }
        }
    }

    if (scene_texture != nullptr)
    {
        command_allocator->Reset();
        command_list_direct->Reset(command_allocator.Get(), nullptr);

        transition_resource(
            command_list_direct.Get(),
            scene_texture->get_underlying(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST
        );

        scene_texture->update(
            command_list_direct.Get(),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            image.data(),
            window->width(),
            window->height(),
            sizeof(u8_four)
        );

        transition_resource(
            command_list_direct.Get(),
            scene_texture->get_underlying(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        device->CreateShaderResourceView(
            scene_texture->get_underlying(),
            &srv_desc,
            srv_descriptor_heap->cpu_handle()
        );

        command_list_direct->Close();
        // Execute eall command now
        ID3D12CommandList* command_lists[] =
        {
            command_list_direct.Get()
        };
        command_queue->execute_command_list(command_lists, 1);
        command_queue->signal();
        command_queue->wait_for_fence();
    }
    else
    {
        scene_texture = std::make_unique<Texture2D>(
            device.Get(),
            command_list_direct.Get(),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            image.data(),
            window->width(),
            window->height(),
            sizeof(u8_four)
        );

        transition_resource(
            command_list_direct.Get(),
            scene_texture->get_underlying(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        device->CreateShaderResourceView(
            scene_texture->get_underlying(),
            &srv_desc,
            srv_descriptor_heap->cpu_handle()
        );
    }
}

bool RTX_Renderer::is_gui_enabled()
{
    return true;
}

bool RTX_Renderer::is_application_initialized()
{
    return app_initialized;
}

void RTX_Renderer::flush()
{
    command_queue->flush();
}

void RTX_Renderer::initialize_raw_input_devices()
{
    RAWINPUTDEVICE rids[1];
    rids[0].usUsagePage = 0x01;
    rids[0].usUsage = 0x02;
    rids[0].dwFlags = RIDEV_NOLEGACY;
    rids[0].hwndTarget = 0;

    ThrowIfFailed(RegisterRawInputDevices(rids, 1, sizeof(rids[0])));
}

void RTX_Renderer::on_key_event(const PackedKeyArguments key_state)
{
    ImGuiIO& io = ImGui::GetIO();
    bool down = key_state.key_state == PackedKeyArguments::Pressed;

    if (key_state.key < 256)
    {
        io.KeysDown[key_state.key] == down;
    }

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

void RTX_Renderer::on_mouse_move(LPARAM lparam)
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
        ImGuiIO& io = ImGui::GetIO();
        switch (raw->data.mouse.usButtonFlags)
        {
        case RI_MOUSE_BUTTON_1_DOWN:
            io.MouseDown[0] = true;
            break;
        case RI_MOUSE_BUTTON_1_UP:
            io.MouseDown[0] = false;
            break;
        default:
            break;
        }
        
        if (keyboard_state.keys[KeyCode::Shift])
        {
            ray_camera->rotate(-raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
        }
    }

    delete[] lpb;
}

void RTX_Renderer::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(scene_pso.Get());
    command_list->SetGraphicsRootSignature(scene_root_signature.Get());

    ID3D12DescriptorHeap* heaps[] = { srv_descriptor_heap->get_underlying() };
    command_list_direct->SetDescriptorHeaps(1, heaps);
    command_list_direct->SetGraphicsRootDescriptorTable(
        0,
        srv_descriptor_heap->gpu_handle()
    );

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, nullptr);
    command_list->DrawInstanced(sizeof(quad_vertices) / sizeof(VertexFormat), 1, 0, 0);
}

void RTX_Renderer::render()
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
    }

    record_command_list(command_list_direct.Get());

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list_direct.Get());
    

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

void RTX_Renderer::resize()
{
    flush();
    
    ThrowIfFailed(command_allocator->Reset());
    ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));

    window->resize();
    swap_chain->resize(device.Get(), window->width(), window->height());

    image.resize(window->width() * window->height());
    scene_texture->resize(
        device.Get(),
        window->width(), window->height(),
        sizeof(u8_four)
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    device->CreateShaderResourceView(
        scene_texture->get_underlying(),
        &srv_desc,
        srv_descriptor_heap->cpu_handle()
    );

    transition_resource(
        command_list_direct.Get(),
        scene_texture->get_underlying(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    command_list_direct->Close();
    // Execute eall command now
    ID3D12CommandList* command_lists[] =
    {
        command_list_direct.Get()
    };
    command_queue->execute_command_list(command_lists, 1);
    command_queue->signal();
    command_queue->wait_for_fence();
}

void RTX_Renderer::update()
{
    compute_delta_time(elapsed_time);

    if (keyboard_state.keys[KeyCode::Shift])
    {
        ray_camera->translate(keyboard_state, elapsed_time);
    }

    if (ray_camera->camera_variables_need_updating())
    {
        ray_camera->reinitialize_camera_variables();
        generate_image();
    }
}

void RTX_Renderer::load_assets()
{
    load_scene_shader_assets();
}

void RTX_Renderer::load_scene_shader_assets()
{
    // Vertex buffer uploading
    {
        vertex_buffer = std::make_unique<DX12Resource>();
        vertex_buffer->upload(device.Get(), command_list_direct.Get(),
            (float*)quad_vertices,
            sizeof(quad_vertices)
        );

        vertex_buffer_view.BufferLocation = vertex_buffer->gpu_virtual_address();
        vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/03_global_illumination/shaders/quad_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/03_global_illumination/shaders/quad_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[2] =
    {
        { 
            "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
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

    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    // Three float4x4
    root_parameters[0].InitAsDescriptorTable(1, &srv_range, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(
        1, root_parameters,
        1, samplers,
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

}