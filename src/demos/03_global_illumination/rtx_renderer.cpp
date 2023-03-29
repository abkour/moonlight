#include "rtx_renderer.hpp"
#include "../../utility/random_number.hpp"

#include "tbb/blocked_range.h"
#include "tbb/blocked_range2d.h"
#include "tbb/parallel_for.h"

#include <chrono>
#include <numeric>  // for std::iota

using namespace DirectX;
using namespace Microsoft::WRL;

#include "texture_image.hpp"
#include "texture_single.hpp"

namespace moonlight {

#define IMGUI_DESC_INDEX            0
#define SRV_BVHNODES_INDEX          1
#define SRV_TRIANGLES_INDEX         2
#define SRV_TRIANGLE_INDICES_INDEX  3
#define SRV_RWTEXTURE_INDEX         4
#define UAV_RWTEXTURE_INDEX         5
#define NUM_DESCRIPTORS             6

struct CS_RayCameraFormat
{
    Vector2<uint32_t> resolution;
    float dummy0, dummy1;
    Vector3<float> eyepos;
    float dummy2;
    Vector3<float> eyedir;
    float dummy3;
    Vector3<float> shiftx;
    float dummy4;
    Vector3<float>shifty;
    float dummy5;
    Vector3<float>topLeftPixel;
    float dummy6;
};

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
    m_window = std::make_unique<Window>(
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
    m_device = _pimpl_create_device(most_sutiable_adapter);
    m_command_queue = std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_allocator = _pimpl_create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_list_direct = _pimpl_create_command_list(m_device, m_command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_swap_chain = std::make_unique<SwapChain>(
        m_device.Get(),
        m_command_queue->get_underlying(),
        m_window->width(),
        m_window->height(),
        m_window->handle
    );

    m_scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), 
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 
        NUM_DESCRIPTORS
    );

    m_ray_camera = std::make_unique<RayCamera>(
        Vector2<uint32_t>(m_window->width(), m_window->height())
    );

    m_ray_camera->initializeVariables(
        Vector3<float>(0.11f, 0.84f, 7.74),
        normalize(Vector3<float>(0.03f, -0.07f, -1.00)),
        45,
        1
    );

    m_ray_camera->set_movement_speed(10.f);
    gui.m_tracing_method = MultiThreaded;

    m_image.resize(m_window->width() * m_window->height());

    // Compute related
    m_compute_command_queue =
        std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_compute_command_allocator = 
        _pimpl_create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_compute_command_list = 
        _pimpl_create_command_list(m_device, m_compute_command_allocator, D3D12_COMMAND_LIST_TYPE_COMPUTE);

    load_assets();

    {
        // IMGUI initialization
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(m_window->handle);
        ImGui_ImplDX12_Init(
            m_device.Get(),
            3,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            m_srv_descriptor_heap->get_underlying(),
            m_srv_descriptor_heap->cpu_handle(IMGUI_DESC_INDEX),
            m_srv_descriptor_heap->gpu_handle(IMGUI_DESC_INDEX)
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

void RTX_Renderer::construct_bvh(const char* asset_path)
{
    m_model = std::make_unique<Model>(asset_path);
    m_model->build_bvh();
}

Vector3<float> RTX_Renderer::trace_path(
    Ray& ray,
    ILight* light_source,
    int traversal_depth)
{
    if (traversal_depth <= 0)
    {
        return Vector3<float>(0.f);
    }
    
    IntersectionParams its = m_model->intersect(ray);
    IntersectionParams its_light = light_source->intersect(ray);

    // The light source is hit before the scene geometry.
    if (its_light.is_intersection())
    {
        if (its_light.t < its.t)
        {
            return light_source->albedo();
        }
    }
    if (!its.is_intersection())
    {
        return Vector3<float>(0.f, 0.f, 0.f);
    }

    its.point = ray.o + (ray.t * ray.d);

    uint32_t material_idx = m_model->material_idx(its);
    IMaterial* material = m_model->get_material(material_idx);
    Vector3<float> attenuation = m_model->color_rgb(material_idx);

    Ray scattered;
    material->scatter(scattered, ray, its);

    return 
        attenuation * 
        material->scattering_pdf(scattered, its) *
        trace_path(scattered, light_source, traversal_depth - 1) /
        material->pdf(scattered, its);
}

void RTX_Renderer::generate_image()
{
    if (!gui.m_asset_loaded)
    {
        return;
    }

    if (gui.m_tracing_method == TracingMethod::ComputeShader)
    {
        dispatch_compute_shader();
        return;
    }

    if (gui.m_tracing_method == TracingMethod::SingleThreaded)
    {
        generate_image_st();
    } 
    else if(gui.m_tracing_method == TracingMethod::MultiThreaded)
    {
        if (gui.m_enable_path_tracing & 1)
        {
            static int i = 0;
            static LoggingFile logger("n_generate.txt", LoggingFile::Truncate);
            logger << i++;
            generate_image_mt_pt();
            logger << ": finished\n";
        }
        else
        {
            generate_image_mt();
        }
    }
    
    upload_to_texture();
}

void RTX_Renderer::generate_image_mt()
{
    tbb::parallel_for(
        tbb::blocked_range2d<uint32_t>(0, m_window->height(), 0, m_window->width()),
            [this](tbb::blocked_range2d<uint32_t> r)
        {
            for (uint32_t x = r.cols().begin(); x < r.cols().end(); ++x)
            {
                for (uint32_t y = r.rows().begin(); y < r.rows().end(); ++y)
                {
                    std::size_t idx = y * m_window->width();
                    idx += (m_window->width() - 1) - x;
                    
                    auto ray = m_ray_camera->getRay({ x, y });
                    IntersectionParams intersect = m_model->intersect(ray);

                    //IntersectionParams intersect =
                    //    intersect_all_triangles(ray, m_mesh.get(), m_stride_in_32floats, m_num_triangles);
                    if (intersect.t < std::numeric_limits<float>::max())
                    {
                        uint32_t material_idx = m_model->material_idx(intersect);

                        Vector3<float> diffuse_color;
                        
                        if (m_model->material_flags() & ML_MISC_FLAG_ATTR_VERTEX_NORMAL)
                        {
                            Vector3<float> mat_color = m_model->normal(intersect.triangle_idx);
                            diffuse_color = mat_color;
                        }
                        else
                        {
                            diffuse_color = m_model->color_rgb(material_idx);
                        }

                        diffuse_color *= 255.f;

                        m_image[idx].r = diffuse_color.x;
                        m_image[idx].g = diffuse_color.y;
                        m_image[idx].b = diffuse_color.z;
                        m_image[idx].a = 255;
                    } else
                    {
                        m_image[idx] = u8_four(0, 0, 0, 0);
                    }
                }
            }
        }
    );
}

void RTX_Renderer::generate_image_mt_pt()
{
    ILight* light_source = new AreaLight(
        { 15, 15, 15 },
        { -0.884011, 5.319334, -2.517968 },
        { -0.884011, 5.318497, -3.567968 },
        { 0.415989, 5.318497, -3.567968 },
        { 0.415989, 5.319334, -2.517968 }
    );

    tbb::parallel_for(
        tbb::blocked_range2d<uint32_t>(0, m_window->height(), 0, m_window->width()),
        [&](tbb::blocked_range2d<uint32_t> r)
        {
            for (uint32_t y = r.rows().begin(); y < r.rows().end(); ++y)
            {
                for (uint32_t x = r.cols().begin(); x < r.cols().end(); ++x)
                {
                    std::size_t idx = y * m_window->width();
                    idx += (m_window->width() - 1) - x;

                    auto ray = m_ray_camera->getRay({ x, y });
                    
                    Vector3<float> albedo(0.f);
                    for (int i = 0; i < gui.m_spp; ++i)
                    {
                        albedo += trace_path(ray, light_source, gui.m_num_bounces);
                    }
                    auto scale = 1.f / (float)gui.m_spp;
                    albedo.x = sqrt(scale * albedo.x);
                    albedo.y = sqrt(scale * albedo.y);
                    albedo.z = sqrt(scale * albedo.z);
                    albedo *= 255.f;

                    m_image[idx].r = albedo.x;
                    m_image[idx].g = albedo.y;
                    m_image[idx].b = albedo.z;
                    m_image[idx].a = 255;
                }
            }
        }
    );

    delete light_source;
}

void RTX_Renderer::generate_image_st()
{
    ILight* light_source = new AreaLight(
        { 15, 15, 15 },
        { 0.2300, 1.5800, -0.2200 },
        { 0.2300, 1.5800, 0.1600 },
        { -0.2400, 1.5800, 0.1600 },
        { -0.2400, 1.5800, -0.2200 }
    );

    for (uint16_t y = 0; y < m_window->height(); y += 4)
    {
        for (uint16_t x = 0; x < m_window->width(); x += 4)
        {
            for (uint16_t v = 0; v < 4; ++v)
            {
                for (uint16_t u = 0; u < 4; ++u)
                {
                    std::size_t idx = ((y + v) * m_window->width());
                    idx += (m_window->width() - 1) - (x + u);
                    if (idx >= m_image.size()) break;
                    
                    uint16_t px = x + u;
                    uint16_t py = y + v;
                    auto ray = m_ray_camera->getRay({ px, py });
                    
                    Vector3<float> albedo(0.f);
                    for (int i = 0; i < gui.m_spp; ++i)
                    {
                        albedo += trace_path(ray, light_source, gui.m_num_bounces);
                    }
                    albedo /= gui.m_spp;
                    albedo.x = sqrt(albedo.x);
                    albedo.y = sqrt(albedo.y);
                    albedo.z = sqrt(albedo.z);
                    albedo *= 255.f;

                    m_image[idx].r = albedo.x;
                    m_image[idx].g = albedo.y;
                    m_image[idx].b = albedo.z;
                    m_image[idx].a = 255;
                }
            }
        }
    }

    delete light_source;
}

void RTX_Renderer::upload_to_texture()
{
    if (m_texture_cpu_uploader == nullptr)
    {
        m_texture_cpu_uploader = std::make_unique<CPUGPUTexture2D>(
            m_dst_texture.Get(),
            DXGI_FORMAT_R8G8B8A8_UNORM
        );

        m_texture_cpu_uploader->initialize(
            m_device.Get(),
            m_window->width(), m_window->height(), sizeof(u8_four)
        );
    }

    m_texture_cpu_uploader->upload(
        m_device.Get(),
        m_command_list_direct.Get(),
        m_dst_texture_state,
        m_image.data(),
        m_window->width(),
        m_window->height(),
        sizeof(u8_four)
    );
}

bool RTX_Renderer::is_application_initialized()
{
    return app_initialized;
}

void RTX_Renderer::flush()
{
    m_command_queue->flush();
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

    if (key_state.key < 256)
    {
        io.KeysDown[key_state.key] = key_state.key_state;
    }

    switch (key_state.key_state)
    {
    case PackedKeyArguments::Released:
        m_keyboard_state.reset(key_state.key);
        break;
    case PackedKeyArguments::Pressed:
        m_keyboard_state.set(key_state.key);
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
        case RI_MOUSE_WHEEL:
            io.MouseWheel = (float)(short)raw->data.mouse.usButtonData / WHEEL_DELTA;
            break;
        default:
            break;
        }
        if (m_keyboard_state.keys[KeyCode::Shift])
        {
            m_ray_camera->rotate(raw->data.mouse.lLastX, -raw->data.mouse.lLastY);
        }
    }

    delete[] lpb;
}

void RTX_Renderer::on_resource_invalidation()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(
        m_dst_texture.Get(),
        nullptr,
        &uav_desc,
        m_srv_descriptor_heap->cpu_handle(UAV_RWTEXTURE_INDEX)
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(
        m_dst_texture.Get(),
        &srv_desc,
        m_srv_descriptor_heap->cpu_handle(SRV_RWTEXTURE_INDEX)
    );
}

void RTX_Renderer::on_switch_tracing_method(TracingMethod prev_tracing_method)
{
    constexpr D3D12_RESOURCE_STATES cpu_state = D3D12_RESOURCE_STATE_COPY_DEST;
    constexpr D3D12_RESOURCE_STATES gpu_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    if ((prev_tracing_method == TracingMethod::SingleThreaded ||
        prev_tracing_method == TracingMethod::MultiThreaded)  &&
        gui.m_tracing_method == TracingMethod::ComputeShader)
    {
        transition_resource(
            m_command_list_direct.Get(),
            m_dst_texture.Get(),
            cpu_state,
            gpu_state
        );

        m_dst_texture_state = gpu_state;
    }
    else if (prev_tracing_method == TracingMethod::ComputeShader  &&
            (gui.m_tracing_method    == TracingMethod::SingleThreaded ||
            gui.m_tracing_method     == TracingMethod::MultiThreaded))
    {
        transition_resource(
            m_command_list_direct.Get(),
            m_dst_texture.Get(),
            gpu_state,
            cpu_state
        );

        m_dst_texture_state = cpu_state;
    }
}

void RTX_Renderer::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_scene_pso.Get());
    command_list->SetGraphicsRootSignature(m_scene_root_signature.Get());
    
    transition_resource(
        command_list,
        m_dst_texture.Get(),
        m_dst_texture_state,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
    command_list->SetDescriptorHeaps(1, heaps);
    command_list->SetGraphicsRootDescriptorTable(
        0,
        m_srv_descriptor_heap->gpu_handle(SRV_RWTEXTURE_INDEX)
    );

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, nullptr);
    command_list->DrawInstanced(sizeof(quad_vertices) / sizeof(VertexFormat), 1, 0, 0);

    transition_resource(
        command_list,
        m_dst_texture.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        m_dst_texture_state
    );
}

void RTX_Renderer::dispatch_compute_shader()
{
    constexpr uint32_t BLOCK_SIZE = 8;
    const uint32_t thread_x = m_window->width() / BLOCK_SIZE;
    const uint32_t thread_y = m_window->height() / BLOCK_SIZE;
    const uint32_t thread_z = 1;
    const Vector2<float> texel_size(1.f / m_window->width(), 1.f / m_window->height());
    
    m_compute_command_list->SetPipelineState(m_cs_pso.Get());
    m_compute_command_list->SetComputeRootSignature(m_cs_root_signature.Get());
    
    ID3D12DescriptorHeap* heaps[] = { 
        m_srv_descriptor_heap->get_underlying(),
    };
    m_compute_command_list->SetDescriptorHeaps(_countof(heaps), heaps);

    m_compute_command_list->SetComputeRootDescriptorTable(
        0, m_srv_descriptor_heap->gpu_handle(SRV_BVHNODES_INDEX)
    );
    m_compute_command_list->SetComputeRootDescriptorTable(
        1, m_srv_descriptor_heap->gpu_handle(UAV_RWTEXTURE_INDEX)
    );
    m_compute_command_list->SetComputeRoot32BitConstants(
        2, 1, temp_address(m_model->stride()), 0
    );

    CS_RayCameraFormat sub_raycamera;
    sub_raycamera.resolution = m_ray_camera->resolution;
    sub_raycamera.eyepos = m_ray_camera->eyepos;
    sub_raycamera.eyedir = m_ray_camera->eyedir;
    sub_raycamera.shiftx = m_ray_camera->shiftx;
    sub_raycamera.shifty = m_ray_camera->shifty;
    sub_raycamera.topLeftPixel = m_ray_camera->topLeftPixel;

    m_compute_command_list->SetComputeRoot32BitConstants(
        3, sizeof(CS_RayCameraFormat) / sizeof(float), &sub_raycamera, 0
    );

    m_compute_command_list->Dispatch(thread_x, thread_y, thread_z);

    m_compute_command_list->Close();
    ID3D12CommandList* command_lists[] =
    {
        m_compute_command_list.Get()
    };

    m_compute_command_queue->execute_command_list(command_lists, 1);
    m_compute_command_queue->signal();
    m_compute_command_queue->wait_for_fence();

    ThrowIfFailed(m_compute_command_allocator->Reset());
    ThrowIfFailed(m_compute_command_list->Reset(m_compute_command_allocator.Get(), nullptr));
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    m_device->CreateUnorderedAccessView(
        m_dst_texture.Get(),
        nullptr,
        &uav_desc,
        m_srv_descriptor_heap->cpu_handle(UAV_RWTEXTURE_INDEX)
    );
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(
        m_dst_texture.Get(),
        &srv_desc,
        m_srv_descriptor_heap->cpu_handle(SRV_RWTEXTURE_INDEX)
    );
}

void RTX_Renderer::record_gui_commands(ID3D12GraphicsCommandList* command_list)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        static int open_file_browser = 0;

        ImGui::Begin("Moonlight");
        if (ImGui::Button("Open/Close File Browser"))
        {
            open_file_browser++;
        }

        if (open_file_browser & 1)
        {
            std::string default_path = std::string(ROOT_DIRECTORY_ASCII) + "/assets";
            static AssetFileBrowser file_browser(default_path.c_str());
            m_asset_path = file_browser.display();
        }

        ImGui::DragInt("spp", &gui.m_spp, 1, 1, 250);
        ImGui::DragInt("bounces", &gui.m_num_bounces, 1, 1, 16);

        ImGui::Text("Choose a threading model");
        {
            // Choose threading model
            const char* threading_names[] =
            {
                "\tCPU-ST", 
                "\tCPU-MT",
                "\tGPU-CS"
            };
            
            TracingMethod prev_tracing_method = gui.m_tracing_method;
            for (unsigned int n = 0; n < _countof(threading_names); n++)
            {
                if (ImGui::Selectable(threading_names[n], gui.m_tracing_method == n))
                    gui.m_tracing_method = TracingMethod(n);
            }

            if (prev_tracing_method != gui.m_tracing_method)
            {
                on_switch_tracing_method(prev_tracing_method);
            }
        }

        if (ImGui::Button("Enable path tracing"))
        {
            gui.m_enable_path_tracing++;
        }

        if (gui.m_enable_path_tracing & 1)
        {
            ImGui::SameLine();
            if(ImGui::Button("Generate New Image"))
            {
                gui.m_generate_new_image = true;
            }
        }

        Vector3<float> cam_pos = m_ray_camera->eyepos;
        Vector3<float> cam_dir = m_ray_camera->eyedir;
        ImGui::Text("Pos: ");
        ImGui::SameLine();
        ImGui::Text("(%.2f, %.2f, %.2f)\n", cam_pos.x, cam_pos.y, cam_pos.z);

        ImGui::Text("Dir: ");
        ImGui::SameLine();
        ImGui::Text("(%.2f, %.2f, %.2f)\n", cam_dir.x, cam_dir.y, cam_dir.z);

        ImGui::Text("\nApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_command_list_direct.Get());
}

void RTX_Renderer::render()
{
    // Clear
    {
        m_swap_chain->transition_to_rtv(m_command_list_direct.Get());

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.05f, 0.05f, 0.05f, 1.f };
        m_command_list_direct->ClearRenderTargetView(
            m_swap_chain->backbuffer_rtv_descriptor_handle(
                m_swap_chain->current_backbuffer_index()),
            clear_color, 
            0, 
            NULL
        );
    }

    if (gui.m_asset_loaded)
    {
        record_command_list(m_command_list_direct.Get());
    }
    else
    {
        uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
        D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
            m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

        ID3D12DescriptorHeap* heaps[] = { m_srv_descriptor_heap->get_underlying() };
        m_command_list_direct->SetDescriptorHeaps(1, heaps);

        m_command_list_direct->RSSetViewports(1, &m_viewport);
        m_command_list_direct->RSSetScissorRects(1, &m_scissor_rect);
        m_command_list_direct->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, nullptr);
    }
    record_gui_commands(m_command_list_direct.Get());

    // Present
    {
        m_swap_chain->transition_to_present(m_command_list_direct.Get());

        m_command_list_direct->Close();
        ID3D12CommandList* command_lists[] =
        {
            m_command_list_direct.Get()
        };

        m_command_queue->execute_command_list(command_lists, 1);
        m_command_queue->signal();
        m_command_queue->wait_for_fence();
        m_swap_chain->present();
    }

    ThrowIfFailed(m_command_allocator->Reset());
    ThrowIfFailed(m_command_list_direct->Reset(m_command_allocator.Get(), NULL));

}

void RTX_Renderer::resize()
{
    flush();
    
    m_window->resize();
    m_swap_chain->resize(m_device.Get(), m_window->width(), m_window->height());
    m_image.resize(m_window->width() * m_window->height());
    {
        // resize the dst_texture
        D3D12_RESOURCE_DESC rsc_desc = {};
        rsc_desc.DepthOrArraySize = 1;
        rsc_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rsc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rsc_desc.MipLevels = 1;
        rsc_desc.Width = m_window->width();
        rsc_desc.Height = m_window->height();
        rsc_desc.SampleDesc = { 1, 0 };
        rsc_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ThrowIfFailed(m_device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
            D3D12_HEAP_FLAG_NONE,
            &rsc_desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&m_dst_texture)
        ));

        on_resource_invalidation();
        
        if (gui.m_tracing_method == TracingMethod::SingleThreaded ||
            gui.m_tracing_method == TracingMethod::MultiThreaded)
        {
            transition_resource(
                m_command_list_direct.Get(),
                m_dst_texture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COPY_DEST
            );
        }
    }
    
    if (m_texture_cpu_uploader)
    {
        m_texture_cpu_uploader->resize(
            m_device.Get(),
            m_dst_texture.Get(),
            m_window->width(), m_window->height(),
            sizeof(u8_four)
        );
    }

    m_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    // Resizing requires retracing the scene
    generate_image();
}

void RTX_Renderer::update()
{
    compute_delta_time(m_elapsed_time);

    if (m_asset_path != nullptr)
    {
        construct_bvh(m_asset_path);
        initialize_shader_resources();
        
        gui.m_asset_loaded = true;
        m_asset_path = nullptr;

        generate_image();

        return;
    }

    if (m_keyboard_state.keys[KeyCode::Shift])
    {
        m_ray_camera->translate(m_keyboard_state, m_elapsed_time);
    }

    if (gui.m_generate_new_image)
    {
        generate_image();
        gui.m_generate_new_image = false;
    }

    if (m_ray_camera->camera_variables_need_updating())
    {
        m_ray_camera->reinitialize_camera_variables();
        generate_image();
    }
}

void RTX_Renderer::load_assets()
{
    //initialize_shader_resources();
    load_scene_shader_assets();
    initialize_cs_pipeline();
}

void RTX_Renderer::initialize_shader_resources()
{
    // Vertex buffer uploading
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            (float*)quad_vertices,
            sizeof(quad_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        m_vertex_buffer_view.StrideInBytes = sizeof(VertexFormat);
    }

    // Initialize m_uav_bvhnodes_rsc
    {
        m_uav_bvhnodes_rsc = std::make_unique<DX12Resource>();
        m_uav_bvhnodes_rsc->upload(
            m_device.Get(),
            m_command_list_direct.Get(),
            m_model->bvh_get_raw_nodes(),
            sizeof(BVHNode) * m_model->bvh_nodes_used(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.NumElements = m_model->bvh_nodes_used();
        srv_desc.Buffer.StructureByteStride = sizeof(BVHNode);

        m_device->CreateShaderResourceView(
            m_uav_bvhnodes_rsc->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle(SRV_BVHNODES_INDEX)
        );
    }

    // Initialize m_uav_tris_rsc
    {
        m_uav_tris_rsc = std::make_unique<DX12Resource>();
        m_uav_tris_rsc->upload(
            m_device.Get(),
            m_command_list_direct.Get(),
            m_model->raw_mesh(),
            sizeof(float) * m_model->num_elements(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.NumElements = m_model->num_elements();
        srv_desc.Buffer.StructureByteStride = sizeof(float);

        m_device->CreateShaderResourceView(
            m_uav_tris_rsc->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle(SRV_TRIANGLES_INDEX)
        );
    }

    // Initialize m_uav_tris_indices_rsc
    {
        m_uav_tris_indices_rsc = std::make_unique<DX12Resource>();
        m_uav_tris_indices_rsc->upload(
            m_device.Get(),
            m_command_list_direct.Get(),
            m_model->bvh_get_raw_indices(),
            sizeof(uint32_t) * m_model->num_triangles(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
        );

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.NumElements = m_model->num_triangles();
        srv_desc.Buffer.StructureByteStride = sizeof(uint32_t);

        m_device->CreateShaderResourceView(
            m_uav_tris_indices_rsc->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle(SRV_TRIANGLE_INDICES_INDEX)
        );
    }

    // Initialize m_dst_texture
    {
        D3D12_RESOURCE_DESC rsc_desc = {};
        rsc_desc.DepthOrArraySize = 1;
        rsc_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        rsc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rsc_desc.MipLevels = 1;
        rsc_desc.Width = m_window->width();
        rsc_desc.Height = m_window->height();
        rsc_desc.SampleDesc = { 1, 0 };
        rsc_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ThrowIfFailed(m_device->CreateCommittedResource(
            temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
            D3D12_HEAP_FLAG_NONE,
            &rsc_desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&m_dst_texture)
        ));

        on_resource_invalidation();

        if (gui.m_tracing_method == TracingMethod::SingleThreaded ||
            gui.m_tracing_method == TracingMethod::MultiThreaded)
        {
            transition_resource(
                m_command_list_direct.Get(),
                m_dst_texture.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COPY_DEST
            );

            m_dst_texture_state = D3D12_RESOURCE_STATE_COPY_DEST;
        } else
        {
            m_dst_texture_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
    }
}

void RTX_Renderer::load_scene_shader_assets()
{
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
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
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
        _countof(root_parameters), root_parameters,
        _countof(samplers), samplers,
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

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&m_scene_root_signature)
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
    scene_pipeline_state_stream.root_signature = m_scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_scene_pso)));
}

void RTX_Renderer::initialize_cs_pipeline()
{
    ComPtr<ID3DBlob> cs_blob;
    {
        std::wstring cspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/03_global_illumination/shaders/bvh_intersect_cs_v100.cso";
        ThrowIfFailed(D3DReadFileToBlob(cspath.c_str(), &cs_blob));
    }

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_NONE;
    
    CD3DX12_DESCRIPTOR_RANGE1 srv_range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE };
    CD3DX12_DESCRIPTOR_RANGE1 uav_range{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE };
    CD3DX12_ROOT_PARAMETER1 root_parameters[4];
    root_parameters[0].InitAsDescriptorTable(1, &srv_range);
    root_parameters[1].InitAsDescriptorTable(1, &uav_range);
    root_parameters[2].InitAsConstants(sizeof(unsigned), 0);
    root_parameters[3].InitAsConstants(sizeof(CS_RayCameraFormat) / sizeof(float), 1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init_1_1(
        _countof(root_parameters), root_parameters,
        0, nullptr,
        root_signature_flags
    );

    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));
    
    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&m_cs_root_signature)
    ));

    D3D12_COMPUTE_PIPELINE_STATE_DESC pss_desc = {};
    pss_desc.CS = CD3DX12_SHADER_BYTECODE(cs_blob.Get());
    pss_desc.pRootSignature = m_cs_root_signature.Get();

    ThrowIfFailed(m_device->CreateComputePipelineState(&pss_desc, IID_PPV_ARGS(&m_cs_pso)));
}

}