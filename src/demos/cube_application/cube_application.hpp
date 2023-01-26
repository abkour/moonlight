#pragma once
#include "../../application.hpp"
#include "../../dx12_wrapper.hpp"
#include "../../project_defines.hpp"

#include <DirectXMath.h>

namespace moonlight {

class CubeApplication : public IApplication {

public:

	CubeApplication(HINSTANCE hinstance);

	~CubeApplication() {}

	bool is_application_initialized() override;

	void flush() override;

	void update() override;

	void render() override;

	void resize() override;

private:

	static constexpr uint16_t window_width = 1024;
	static constexpr uint16_t window_height = 720;

private:

	Microsoft::WRL::ComPtr<ID3D12Device2> device;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
	void _pimpl_create_command_queue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain;
	void _pimpl_create_swap_chain(
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue
	);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
	void _pimpl_create_rtv_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap;
	void _pimpl_create_dsv_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
	void _pimpl_create_dsv(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap
	);

	Microsoft::WRL::ComPtr<ID3D12Resource> backbuffers[3];
	void _pimpl_create_backbuffers(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap
	);
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	void _pimpl_create_command_allocator(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type
	);

	Microsoft::WRL::ComPtr<ID3D12CommandList> command_list_copy;
	void _pimpl_create_command_list_copy(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE type
	);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
	void _pimpl_create_command_list(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE type
	);

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	void _pimpl_create_fence(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	HANDLE fence_event;
	void _pimpl_create_fence_event();

	//
	// Triangle
	//
	void load_assets();

	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor_rect;

	DirectX::XMMATRIX mvp_matrix;

private:

	void command_queue_signal(uint64_t fence_value);

	void flush_command_queue();

	void wait_for_fence(uint64_t fence_value);

	void transition_resource(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES before_state,
		D3D12_RESOURCE_STATES after_state
	);

private:

	uint64_t fence_value = 0;
	bool app_initialized = false;
};

}