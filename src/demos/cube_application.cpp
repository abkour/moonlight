#include "cube_application.hpp"

#include <d3d12.h>
#include <d3dcompiler.h>

#include <algorithm>

using namespace DirectX;
using namespace Microsoft::WRL;

// This gets around the problem of taking the address of a rvalue
// This works on architectures where the functions return their value on the stack. (x86)
// Doesn't work on ARM.
template<typename T>
T* get_temporary_address(T&& x)
{
	return &x;
}

namespace moonlight {

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

struct PipelineStateStream
{
	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
	CD3DX12_PIPELINE_STATE_STREAM_VS VS;
	CD3DX12_PIPELINE_STATE_STREAM_PS PS;
	CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
} pipeline_state_stream;

// Helper functions
void CubeApplication::load_content()
{
	ComPtr<ID3D12Resource> intermediate_vertex_buffer;
	
	auto command_list = command_queue_copy->get_command_list();

	update_buffer_resource(
		command_list,
		&vertex_buffer,
		&intermediate_vertex_buffer,
		_countof(vertices),
		sizeof(VertexPosColor),
		vertices
	);

	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.SizeInBytes = sizeof(vertices);
	vertex_buffer_view.StrideInBytes = sizeof(VertexPosColor);

	ComPtr<ID3D12Resource> intermediate_index_buffer;
	
	update_buffer_resource(
		command_list,
		&index_buffer,
		&intermediate_index_buffer,
		_countof(indicies),
		sizeof(WORD),
		indicies
	);

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
	index_buffer_view.SizeInBytes = sizeof(indicies);

	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
	dsv_heap_desc.NumDescriptors = 1;
	dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&dsv_heap)));
	
	ComPtr<ID3DBlob> vertex_shader_blob;
	ComPtr<ID3DBlob> pixel_shader_blob;

	{
		const wchar_t* vspath = L"C://Users//flora//dev//moonlight//src//demos//shaders//cube_vs.cso";
		const wchar_t* pspath = L"C://Users//flora//dev//moonlight//src//demos//shaders//cube_ps.cso";
		ThrowIfFailed(D3DReadFileToBlob(vspath, &vertex_shader_blob));
		ThrowIfFailed(D3DReadFileToBlob(pspath, &pixel_shader_blob));
	}

	D3D12_INPUT_ELEMENT_DESC input_layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature
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
	root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 0, nullptr, root_signature_flags);

	// Serialize the root signature
	ComPtr<ID3DBlob> root_signature_blob;
	ComPtr<ID3DBlob> error_blob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
		&root_signature_desc,
		feature_data.HighestVersion,
		&root_signature_blob,
		&error_blob)
	);

	// Create the root signature
	ThrowIfFailed(device->CreateRootSignature(
		0, 
		root_signature_blob->GetBufferPointer(),
		root_signature_blob->GetBufferSize(), 
		IID_PPV_ARGS(&root_signature))
	);
	
	D3D12_RT_FORMAT_ARRAY rtv_formats = {};
	rtv_formats.NumRenderTargets = 1;
	rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipeline_state_stream.pRootSignature = root_signature.Get();
	pipeline_state_stream.InputLayout = { input_layout, _countof(input_layout) };
	pipeline_state_stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_state_stream.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_blob.Get());
	pipeline_state_stream.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_blob.Get());
	pipeline_state_stream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipeline_state_stream.RTVFormats = rtv_formats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipeline_state_stream_desc = {
		sizeof(PipelineStateStream),
		& pipeline_state_stream
	};

	ThrowIfFailed(device->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipeline_state)));
	
	uint64_t fence_value = command_queue_copy->execute_command_list(command_list);
	command_queue_copy->wait_for_fence_value(fence_value);

	content_loaded = true;

	resize_depth_buffer(window->width(), window->height());
}

void CubeApplication::render()
{
	UINT current_backbuffer_idx = swap_chain->get_backbuffer_index();
	
	auto command_list = command_queue_direct->get_command_list();
	auto backbuffer = swap_chain->get_backbuffer(current_backbuffer_idx);
	
	auto rtv_desc_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		descriptor_heap->GetCPUDescriptorHandleForHeapStart(), 
		current_backbuffer_idx,
		rtv_desc_size
	);

	auto dsv = dsv_heap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render target
	{
		transition_resource(
			command_list,
			backbuffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);

		FLOAT clear_color[] = { 0.9f, 0.6f, 1.f, 1.f };

		clear_rtv(command_list, rtv, clear_color);
		clear_depth(command_list, dsv);
	}

	command_list->SetPipelineState(pipeline_state.Get());
	command_list->SetGraphicsRootSignature(root_signature.Get());

	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	command_list->IASetIndexBuffer(&index_buffer_view);

	command_list->RSSetViewports(1, &viewport);
	command_list->RSSetScissorRects(1, &scissor_rect);

	command_list->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX mvp_matrix = XMMatrixMultiply(model_matrix, view_matrix);
	mvp_matrix = XMMatrixMultiply(mvp_matrix, projection_matrix);
	command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);

	command_list->DrawIndexedInstanced(_countof(indicies), 1, 0, 0, 0);

	// Present the image to the swapchain
	{
		transition_resource(
			command_list,
			backbuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		fence_values[current_backbuffer_idx] = command_queue_direct->execute_command_list(command_list);

		current_backbuffer_idx = swap_chain->present(window->is_vsync_on(), false);

		command_queue_direct->wait_for_fence_value(fence_values[current_backbuffer_idx]);
	}
}

void CubeApplication::resize_depth_buffer(int width, int height)
{
	if (content_loaded) {
		flush();

		width = std::max(1, width);
		height = std::max(1, height);

		D3D12_CLEAR_VALUE optimized_clear_value = {};
		optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
		optimized_clear_value.DepthStencil = { 1.f, 0 };

		ThrowIfFailed(device->CreateCommittedResource(
			get_temporary_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
			D3D12_HEAP_FLAG_NONE,
			get_temporary_address(CD3DX12_RESOURCE_DESC::Tex2D(
				DXGI_FORMAT_D32_FLOAT,
				width,
				height,
				1,
				0,
				1,
				0,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimized_clear_value,
			IID_PPV_ARGS(&depth_buffer)
		));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(
			depth_buffer.Get(),
			&dsv,
			dsv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}

void CubeApplication::resize_window(int width, int height)
{
	viewport = CD3DX12_VIEWPORT(
		0.f,
		0.f,
		static_cast<float>(width),
		static_cast<float>(height)
	);

	resize_depth_buffer(width, height);
}

void CubeApplication::update()
{
	static auto t0 = std::chrono::high_resolution_clock::now();
	static uint64_t frame_count = 0;
	static float total_time = 0.f;
	frame_count++;
	auto t1 = std::chrono::high_resolution_clock::now();
	total_time += (t1 - t0).count() * 1e-9;
	t0 = t1;
	if (total_time > 1.f) {
		float fps = frame_count / total_time;

		char buffer[512];
		sprintf_s(buffer, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frame_count = 0;
		total_time = 0.f;
	}

	// Update the model matrix
	float angle = static_cast<float>(total_time * 90.f);
	XMVECTOR rotation_axis = XMVectorSet(0, 1, 1, 0);
	model_matrix = XMMatrixRotationAxis(rotation_axis, XMConvertToRadians(angle));

	// Update the view matrix
	XMVECTOR eye_position = XMVectorSet(0, 0, -10, 1);
	XMVECTOR focus_point = XMVectorSet(0, 0, 0, 1);
	XMVECTOR up_direction = XMVectorSet(0, 1, 0, 0);
	view_matrix = XMMatrixLookAtLH(eye_position, focus_point, up_direction);

	// Update the projection matrix
	float aspect_ratio = window->width() / static_cast<float>(window->height());
	projection_matrix = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(fov), 
		aspect_ratio, 
		0.1f, 
		100.f
	);
}

void CubeApplication::clear_depth(
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv,
	FLOAT depth)
{
	command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CubeApplication::clear_rtv(
	ComPtr<ID3D12GraphicsCommandList2> command_list,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv,
	FLOAT* clear_color)
{
	command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
}

void CubeApplication::transition_resource(
	ComPtr<ID3D12GraphicsCommandList2> command_list,
	ComPtr<ID3D12Resource> resource,
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

void CubeApplication::update_buffer_resource(
	ComPtr<ID3D12GraphicsCommandList2> command_list,
	ID3D12Resource** destination_resource,
	ID3D12Resource** intermediate_resource,
	size_t num_elements,
	size_t element_size,
	const void* buffer_data,
	D3D12_RESOURCE_FLAGS flags)
{
	
	size_t buffer_size = num_elements * element_size;

	ThrowIfFailed(device->CreateCommittedResource(
		get_temporary_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		get_temporary_address(CD3DX12_RESOURCE_DESC::Buffer(buffer_size, flags)),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(destination_resource)
	));

	if (buffer_data) {
		ThrowIfFailed(device->CreateCommittedResource(
			get_temporary_address(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
			D3D12_HEAP_FLAG_NONE,
			get_temporary_address(CD3DX12_RESOURCE_DESC::Buffer(buffer_size)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(intermediate_resource)
		));

		D3D12_SUBRESOURCE_DATA sub_resource_data = {};
		sub_resource_data.pData = buffer_data;
		sub_resource_data.RowPitch = buffer_size;
		sub_resource_data.SlicePitch = sub_resource_data.RowPitch;

		UpdateSubresources(
			command_list.Get(),
			*destination_resource,
			*intermediate_resource,
			0,
			0,
			1,
			&sub_resource_data
		);
	}
}

CubeApplication::CubeApplication(HINSTANCE hinstance)
{
	const wchar_t* window_class_name = L"D3D12 Learning Application";
	const wchar_t* window_title = L"D3D12CubeDemo";
	uint32_t width = 1024;
	uint32_t height = 720;

	std::memset(fence_values, 0, sizeof(fence_values));

	scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<float>(width), static_cast<float>(height));
	fov = 45.f;

	content_loaded = false;
	system_initialized = false;

	window = std::make_unique<Window>(
		hinstance,
		window_class_name,
		window_title,
		width,
		height,
		&CubeApplication::WindowMessagingProcess,
		this
	);

	const D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type =
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	const uint8_t n_backbuffers = 3;

	auto most_suitable_adapter = DX12Wrapper::create_adapter();
	device = DX12Wrapper::create_device(most_suitable_adapter);

	command_queue_compute = DX12Wrapper::create_command_queue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	command_queue_copy = DX12Wrapper::create_command_queue(device, D3D12_COMMAND_LIST_TYPE_COPY);
	command_queue_direct = DX12Wrapper::create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	descriptor_heap = DX12Wrapper::create_descriptor_heap(
		device,
		descriptor_type,
		n_backbuffers
	);

	swap_chain = DX12Wrapper::create_swap_chain(
		window,
		device,
		descriptor_heap,
		command_queue_direct
	);

	load_content();

	content_loaded = true;
	system_initialized = true;
}

void CubeApplication::resize()
{
	if (window->resize()) {
		flush();

		for (uint8_t i = 0; i < 3; ++i) {
			swap_chain->release_buffer(i);
			fence_values[i] = fence_values[swap_chain->get_backbuffer_index()];
		}

		swap_chain->resize_buffers(device, descriptor_heap, window->width(), window->height());
	
		// Resize the viewport
		resize_window(window->width(), window->height());
	}
}

void CubeApplication::flush()
{
	command_queue_compute->flush();
	command_queue_copy->flush();
	command_queue_direct->flush();
}

}