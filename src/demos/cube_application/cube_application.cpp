#include "cube_application.hpp"

#include <d3dcompiler.h>

#include <chrono>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace moonlight
{

template<typename T>
T* temp_address(T&& v)
{
	return &v;
}

struct VertexPosColor {
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f),	XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f, 1.0f, -1.0f),		XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f, 1.0f, -1.0f),		XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f),		XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,1.0f),		XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f, 1.0f, 1.0f),		XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f, 1.0f, 1.0f),		XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f, 1.0f),		XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

//
// Public implementation
//
CubeApplication::CubeApplication(HINSTANCE hinstance)
{
	window = std::make_unique<Window>(
		hinstance,
		L"DX12MoonlightApplication",
		L"DX12 Demo",
		window_width,
		window_height,
		&CubeApplication::WindowMessagingProcess,
		this
	);

	ComPtr<IDXGIAdapter4> most_sutiable_adapter = DX12Wrapper::create_adapter();
	device = DX12Wrapper::create_device(most_sutiable_adapter);
	_pimpl_create_command_queue(device);
	_pimpl_create_swap_chain(command_queue);
	_pimpl_create_rtv_descriptor_heap(device);
	_pimpl_create_backbuffers(device, swap_chain, rtv_descriptor_heap);
	_pimpl_create_dsv_descriptor_heap(device);
	_pimpl_create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	_pimpl_create_command_list(device, command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
	_pimpl_create_fence(device);
	_pimpl_create_fence_event();

	scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(window_width), static_cast<float>(window_height));

	// Load assets used for rendering
	load_assets();

	_pimpl_create_dsv(device, dsv_descriptor_heap);

	command_list_direct->Close();
	
	app_initialized = true;
}

bool CubeApplication::is_application_initialized()
{
	return app_initialized;
}

void CubeApplication::update()
{
	static auto t0 = std::chrono::high_resolution_clock::now();
	static float total_time = 0.f;
	auto t1 = std::chrono::high_resolution_clock::now();
	float elapsed_time = (t1 - t0).count() * 1e-9;
	total_time += elapsed_time;
	t0 = t1;

	float angle = static_cast<float>(total_time * 90.f);
	XMVECTOR rotation_axis = XMVectorSet(1, 1, 0, 0);

	XMMATRIX model_matrix = XMMatrixRotationAxis(rotation_axis, XMConvertToRadians(angle));

	XMVECTOR eye_position = XMVectorSet(0, 0, -10, 1);
	XMVECTOR eye_target = XMVectorSet(0, 0, 0, 1);
	XMVECTOR world_up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX view_matrix = XMMatrixLookAtLH(eye_position, eye_target, world_up);

	float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
	XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.f),
		aspect_ratio,
		0.1,
		100.f
	);

	mvp_matrix = XMMatrixMultiply(model_matrix, view_matrix);
	mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);
}

void CubeApplication::render()
{
	ThrowIfFailed(command_allocator->Reset());
	ThrowIfFailed(command_list_direct->Reset(command_allocator.Get(), NULL));
	
	uint8_t backbuffer_idx = swap_chain->GetCurrentBackBufferIndex();
	auto backbuffer = backbuffers[backbuffer_idx];

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
	UINT rtv_inc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	auto dsv_handle = dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

	// Clear
	{
		transition_resource(
			command_list_direct,
			backbuffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		const FLOAT clear_color[] = {0.1f, 0.1f, 0.1f, 1.f};
		command_list_direct->ClearRenderTargetView(
			rtv_handle.Offset(rtv_inc_size * backbuffer_idx), 
			clear_color, 
			0, 
			NULL
		);

		command_list_direct->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
	}

	// Record
	command_list_direct->SetPipelineState(pso.Get());
	command_list_direct->SetGraphicsRootSignature(root_signature.Get());
	command_list_direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list_direct->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	command_list_direct->IASetIndexBuffer(&index_buffer_view);
	command_list_direct->RSSetViewports(1, &viewport);
	command_list_direct->RSSetScissorRects(1, &scissor_rect);
	command_list_direct->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
	command_list_direct->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
	command_list_direct->DrawIndexedInstanced(_countof(indicies), 1, 0, 0, 0);

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

void CubeApplication::resize()
{
}

void CubeApplication::flush()
{
	flush_command_queue();
}

//
// Private implemenatation (Triangle)
//
static struct PipelineStateStream
{
	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
	CD3DX12_PIPELINE_STATE_STREAM_VS vs;
	CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
	CD3DX12_PIPELINE_STATE_STREAM_PS ps;
	CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
} pipeline_state_stream;

void CubeApplication::load_assets()
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
		vertex_buffer_view.StrideInBytes = sizeof(VertexPosColor);
	}

	// Index buffer loading
	{
		ThrowIfFailed(device->CreateCommittedResource(
			temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			temp_address(CD3DX12_RESOURCE_DESC::Buffer(sizeof(indicies))),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&index_buffer)
		));

		UINT32* index_data_begin = nullptr;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(index_buffer->Map(
			0,
			&read_range,
			reinterpret_cast<void**>(&index_data_begin)
		));
		memcpy(index_data_begin, indicies, sizeof(indicies));
		index_buffer->Unmap(0, nullptr);

		index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
		index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
		index_buffer_view.SizeInBytes = sizeof(indicies);
	}

	ComPtr<ID3DBlob> vs_blob;
	ComPtr<ID3DBlob> ps_blob;
	{
		std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//cube_application//shaders//triangle_vs.cso";
		std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"//src//demos//cube_application//shaders//triangle_ps.cso";
		ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
		ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
	}

	D3D12_INPUT_ELEMENT_DESC input_layout[] =
	{
		{ 
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, 
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 
		},
		{
			"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 root_parameters[1];
	root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
	// TODO: Is this call correct, if the highest supported version is 1_0?
	root_signature_desc.Init_1_1(1, root_parameters, 0, nullptr, root_signature_flags);

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
		IID_PPV_ARGS(&root_signature)
	));

	D3D12_RT_FORMAT_ARRAY rtv_formats = {};
	rtv_formats.NumRenderTargets = 1;
	rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
	rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizer_desc.DepthClipEnable = TRUE;
	rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
	pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
	pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	pipeline_state_stream.root_signature = root_signature.Get();
	pipeline_state_stream.rs = rasterizer_desc;
	pipeline_state_stream.rtv_formats = rtv_formats;
	pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());

	D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
		sizeof(PipelineStateStream),
		&pipeline_state_stream
	};

	ThrowIfFailed(device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&pso)));
}

//
// Private Implementation (Constructors)
//
void CubeApplication::_pimpl_create_command_queue(
	Microsoft::WRL::ComPtr<ID3D12Device2> device)
{
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.NodeMask = 0;
	queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));
}

void CubeApplication::_pimpl_create_swap_chain(
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue)
{
	ComPtr<IDXGIFactory4> factory4;
	UINT factory_flags = 0;
#ifdef _DEBUG
	factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory4)));

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width = window_width;
	swap_chain_desc.Height = window_height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.Stereo = FALSE;
	swap_chain_desc.SampleDesc = { 1, 0 };
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 3;
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swap_chain_desc.Flags = 0;

	ComPtr<IDXGISwapChain1> swap_chain1;
	factory4->CreateSwapChainForHwnd(
		command_queue.Get(),
		window->handle,
		&swap_chain_desc,
		NULL,
		NULL,
		&swap_chain1
	);

	ThrowIfFailed(factory4->MakeWindowAssociation(window->handle, DXGI_MWA_NO_ALT_ENTER));

	swap_chain1.As(&swap_chain);
}

void CubeApplication::_pimpl_create_rtv_descriptor_heap(
	Microsoft::WRL::ComPtr<ID3D12Device2> device)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_heap_desc.NodeMask = 0;
	rtv_heap_desc.NumDescriptors = 3;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(device->CreateDescriptorHeap(
		&rtv_heap_desc, 
		IID_PPV_ARGS(&rtv_descriptor_heap
	)));
}

void CubeApplication::_pimpl_create_dsv_descriptor_heap(
	Microsoft::WRL::ComPtr<ID3D12Device2> device)
{
	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
	dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_heap_desc.NumDescriptors = 1;
	dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device->CreateDescriptorHeap(
		&dsv_heap_desc,
		IID_PPV_ARGS(&dsv_descriptor_heap)
	));
}

void CubeApplication::_pimpl_create_dsv(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap)
{
	D3D12_CLEAR_VALUE optimized_clear_value = {};
	optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	optimized_clear_value.DepthStencil = { 1.f, 0 };

	ThrowIfFailed(device->CreateCommittedResource(
		temp_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		temp_address(CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			window_width,
			window_height,
			1, 0, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimized_clear_value,
		IID_PPV_ARGS(&depth_buffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.Texture2D.MipSlice = 0;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	device->CreateDepthStencilView(
		depth_buffer.Get(),
		&dsv_desc,
		dsv_descriptor_heap->GetCPUDescriptorHandleForHeapStart()
	);
}

void CubeApplication::_pimpl_create_backbuffers(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap)
{
	UINT rtv_desc_size = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
		descriptor_heap->GetCPUDescriptorHandleForHeapStart()
	);

	for (uint8_t i = 0; i < 3; ++i)
	{
		ComPtr<ID3D12Resource> backbuffer;
		swap_chain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));

		device->CreateRenderTargetView(
			backbuffer.Get(),
			nullptr,
			rtv_handle
		);

		backbuffers[i] = backbuffer;

		rtv_handle.Offset(rtv_desc_size);
	}
}

void CubeApplication::_pimpl_create_command_allocator(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		type,
		IID_PPV_ARGS(&command_allocator)
	));
}

void CubeApplication::_pimpl_create_command_list_copy(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
	D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandList(
		0,
		type,
		command_allocator.Get(),
		NULL,
		IID_PPV_ARGS(&command_list_copy)
	));
}

void CubeApplication::_pimpl_create_command_list(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
	D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandList(
		0,
		type,
		command_allocator.Get(),
		NULL,
		IID_PPV_ARGS(&command_list_direct)
	));
}

void CubeApplication::_pimpl_create_fence(
	Microsoft::WRL::ComPtr<ID3D12Device2> device)
{
	ThrowIfFailed(device->CreateFence(
		0, 
		D3D12_FENCE_FLAG_NONE, 
		IID_PPV_ARGS(&fence)
	));
}

void CubeApplication::_pimpl_create_fence_event()
{
	fence_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}

//
// Private implementation (Modifiers)
//
void CubeApplication::command_queue_signal(uint64_t fence_value)
{
	ThrowIfFailed(command_queue->Signal(fence.Get(), fence_value));
}

void CubeApplication::flush_command_queue()
{
	++fence_value;
	command_queue_signal(fence_value);
	wait_for_fence(fence_value);
}

void CubeApplication::wait_for_fence(uint64_t fence_value)
{
	if (fence->GetCompletedValue() < fence_value)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
		WaitForSingleObject(fence_event, INFINITE);
	}
}

void CubeApplication::transition_resource(
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

}