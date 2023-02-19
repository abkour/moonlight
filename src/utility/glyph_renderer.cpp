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
        font_descriptor_heap = std::make_unique<DescriptorHeap>(
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

        sprite_batch = std::make_unique<SpriteBatch>(device, resource_upload, pps);;

        auto upload_resource_finished = resource_upload.End(command_queue);
        upload_resource_finished.wait();

        this->viewport = viewport;
        sprite_batch->SetViewport(viewport);
    }

    //
    // Set up the SpriteFont object
    {
        ResourceUploadBatch resource_upload(device);
        resource_upload.Begin();

        sprite_font = std::make_unique<SpriteFont>(
            device,
            resource_upload,
            font_filepath,
            font_descriptor_heap->GetCpuHandle(Descriptors::CourierFont),
            font_descriptor_heap->GetGpuHandle(Descriptors::CourierFont)
        );

        auto upload_resource_finished = resource_upload.End(command_queue);
        upload_resource_finished.wait();
    }
}

GlyphRenderer::~GlyphRenderer()
{
    m_graphicsMemory.reset();
    sprite_batch.reset();
    sprite_font.reset();
    font_descriptor_heap.reset();
}

void GlyphRenderer::render_text(
    ID3D12GraphicsCommandList* command_list,
    ID3D12CommandQueue* command_queue,
    const wchar_t* text,
    const XMFLOAT2 text_pos)
{
    command_list->RSSetViewports(1, &viewport);
    ID3D12DescriptorHeap* font_heaps[] = { font_descriptor_heap->Heap() };
    command_list->SetDescriptorHeaps(1, font_heaps);

    XMFLOAT2 origin;
    XMStoreFloat2(&origin, sprite_font->MeasureString(text) / 2.f);
    sprite_batch->Begin(command_list);
    sprite_font->DrawString(sprite_batch.get(), text, text_pos, Colors::White, 0.f, origin);
    sprite_batch->End();

    m_graphicsMemory->Commit(command_queue);
}


}