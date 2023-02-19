#pragma once 
#include "../../ext/DirectXTK12/Inc/DescriptorHeap.h"
#include "../../ext/DirectXTK12/Inc/ResourceUploadBatch.h"
#include "../../ext/DirectXTK12/Inc/SpriteBatch.h"
#include "../../ext/DirectXTK12/Inc/SpriteFont.h"
#include <memory>
#include <string>

namespace moonlight
{

struct GlyphRenderer
{
	GlyphRenderer(
		ID3D12Device2* device,
		ID3D12CommandQueue* command_queue,
		D3D12_VIEWPORT viewport,
		const wchar_t* font_filepath
	);

	~GlyphRenderer();

	void render_text(
		ID3D12GraphicsCommandList* command_list,
		ID3D12CommandQueue* command_queue,
		const wchar_t* text,
		const DirectX::XMFLOAT2 text_pos
	);

private:

	std::unique_ptr<DirectX::SpriteBatch> sprite_batch;
	std::unique_ptr<DirectX::SpriteFont> sprite_font;
	std::unique_ptr<DirectX::DescriptorHeap> font_descriptor_heap;
	D3D12_VIEWPORT viewport;

	enum Descriptors
	{
		CourierFont,
		Count
	};

	std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
};

}