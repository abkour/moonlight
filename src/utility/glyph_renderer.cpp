#include "glyph_renderer.hpp"

namespace moonlight
{

using namespace DirectX;

GlyphRenderer::GlyphRenderer(
    ID3D12Device2* device, 
    ID3D12CommandQueue* command_queue,
    D3D12_VIEWPORT viewport,
    const wchar_t* font_filepath)
{
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    //
    // Initialize descriptor heap for font
    {
        m_font_descriptor_heap = std::make_unique<DescriptorHeap>(
            device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            Descriptors::Count
        );
    }

    //
    // Set up the SpriteBatch object
    {
        ResourceUploadBatch resource_upload(device);
        resource_upload.Begin();

        RenderTargetState rt_state(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
        SpriteBatchPipelineStateDescription pps(rt_state);

        m_sprite_batch = std::make_unique<SpriteBatch>(device, resource_upload, pps);;

        auto upload_resource_finished = resource_upload.End(command_queue);
        upload_resource_finished.wait();

        m_viewport = viewport;
        m_sprite_batch->SetViewport(viewport);
    }

    //
    // Set up the SpriteFont object
    {
        ResourceUploadBatch resource_upload(device);
        resource_upload.Begin();

        m_sprite_font = std::make_unique<SpriteFont>(
            device,
            resource_upload,
            font_filepath,
            m_font_descriptor_heap->GetCpuHandle(Descriptors::CourierFont),
            m_font_descriptor_heap->GetGpuHandle(Descriptors::CourierFont)
        );

        auto upload_resource_finished = resource_upload.End(command_queue);
        upload_resource_finished.wait();
    }
}

GlyphRenderer::~GlyphRenderer()
{
    m_graphicsMemory.reset();
    m_sprite_batch.reset();
    m_sprite_font.reset();
    m_font_descriptor_heap.reset();
}

void GlyphRenderer::render_text(
    ID3D12GraphicsCommandList* command_list,
    ID3D12CommandQueue* command_queue,
    const wchar_t* text,
    const XMFLOAT2 text_pos)
{
    command_list->RSSetViewports(1, &m_viewport);
    ID3D12DescriptorHeap* font_heaps[] = { m_font_descriptor_heap->Heap() };
    command_list->SetDescriptorHeaps(1, font_heaps);

    XMFLOAT2 origin;
    XMStoreFloat2(&origin, m_sprite_font->MeasureString(text) / 2.f);
    m_sprite_batch->Begin(command_list);
    m_sprite_font->DrawString(m_sprite_batch.get(), text, text_pos, Colors::White, 0.f, origin);
    m_sprite_batch->End();

    m_graphicsMemory->Commit(command_queue);
}


}