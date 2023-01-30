#pragma once
#include "../ext/d3dx12.h"

#include "gameplay_system.hpp"
#include "window.hpp"

#include <memory>

namespace moonlight {

class IApplication {

public:

	IApplication() {}
	IApplication(HINSTANCE hinstance);

	virtual ~IApplication() = 0 {}

	virtual bool is_application_initialized() = 0;

	virtual void flush() = 0;

	virtual void update() = 0;
	
	virtual void render() = 0;

	virtual void resize() = 0;

	void run();

	static LRESULT CALLBACK WindowMessagingProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:

	virtual Microsoft::WRL::ComPtr<IDXGIAdapter4> _pimpl_create_adapter();

	virtual Microsoft::WRL::ComPtr<ID3D12Device2> _pimpl_create_device(
		Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter
	);

	virtual Microsoft::WRL::ComPtr<ID3D12CommandQueue> _pimpl_create_command_queue(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	virtual Microsoft::WRL::ComPtr<IDXGISwapChain4> _pimpl_create_swap_chain(
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue,
		const uint16_t window_width,
		const uint16_t window_height
	);

	virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_rtv_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_dsv_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	virtual Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _pimpl_create_srv_descriptor_heap(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	virtual Microsoft::WRL::ComPtr<ID3D12Resource> _pimpl_create_dsv(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap,
		const uint16_t window_width,
		const uint16_t window_height
	);

	virtual void _pimpl_create_backbuffers(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap,
		Microsoft::WRL::ComPtr<ID3D12Resource>* backbuffers,
		uint8_t n_backbuffers
	);

	virtual Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _pimpl_create_command_allocator(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		D3D12_COMMAND_LIST_TYPE type
	);

	virtual Microsoft::WRL::ComPtr<ID3D12CommandList> _pimpl_create_command_list_copy(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE type
	);

	virtual Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _pimpl_create_command_list(
		Microsoft::WRL::ComPtr<ID3D12Device2> device,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator,
		D3D12_COMMAND_LIST_TYPE type
	);

	virtual Microsoft::WRL::ComPtr<ID3D12Fence> _pimpl_create_fence(
		Microsoft::WRL::ComPtr<ID3D12Device2> device
	);

	HANDLE _pimpl_create_fence_event();

protected:

	//std::unique_ptr<GameplaySystem> gameplay_system;
	std::unique_ptr<Window> window;
};

}