#include "cube_application.hpp"

namespace moonlight {

CubeApplication::CubeApplication(HINSTANCE hinstance)
{
	const wchar_t* window_class_name = L"D3D12 Learning Application";
	const wchar_t* window_title = L"D3D12CubeDemo";
	uint32_t width = 1024;
	uint32_t height = 720;

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

	const D3D12_COMMAND_LIST_TYPE command_list_type =
		D3D12_COMMAND_LIST_TYPE_DIRECT;

	const D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type =
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	const uint8_t n_backbuffers = 3;

	auto most_suitable_adapter = DX12Wrapper::create_adapter();
	device = DX12Wrapper::create_device(most_suitable_adapter);

	command_queue = DX12Wrapper::create_command_queue(device, command_list_type);

	descriptor_heap = DX12Wrapper::create_descriptor_heap(
		device,
		descriptor_type,
		n_backbuffers
	);

	swap_chain = DX12Wrapper::create_swap_chain(
		window,
		device,
		descriptor_heap,
		command_queue
	);

	for (uint8_t i = 0; i < n_backbuffers; ++i) {
		command_allocators[i] = DX12Wrapper::create_command_allocator(device, command_list_type);
	}

	command_list = DX12Wrapper::create_command_list(
		device, 
		command_allocators[swap_chain->get_backbuffer_index()], 
		command_list_type
	);

	system_initialized = true;

	gameplay_system = std::make_unique<GameplaySystem>();
}

void CubeApplication::flush() 
{
	uint8_t current_backbuffer_idx = swap_chain->get_backbuffer_index();
	command_queue->flush(current_backbuffer_idx);
}

void CubeApplication::render() 
{
	uint8_t current_backbuffer_idx = swap_chain->get_backbuffer_index();
	auto command_allocator = command_allocators[current_backbuffer_idx];
	auto backbuffer = swap_chain->get_backbuffer(current_backbuffer_idx);

	command_allocator->Reset();
	command_list->Reset(command_allocator.Get(), nullptr);

	// Clear the render target
	{
		CD3DX12_RESOURCE_BARRIER barrier =
			CD3DX12_RESOURCE_BARRIER::Transition(
				backbuffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET);

		command_list->ResourceBarrier(1, &barrier);

		FLOAT clear_color[] = { 1.f, 0.3f, 0.15f, 1.f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
			descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
			current_backbuffer_idx,
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		command_list->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
	}

	// Present to the swap chain
	{
		CD3DX12_RESOURCE_BARRIER barrier =
			CD3DX12_RESOURCE_BARRIER::Transition(
				backbuffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PRESENT);

		command_list->ResourceBarrier(1, &barrier);

		ThrowIfFailed(command_list->Close());

		ID3D12CommandList* const command_lists[] =
		{
			command_list.Get()
		};

		command_queue->execute(command_lists, _countof(command_lists));

		BOOL tearing_supported = FALSE;
		swap_chain->present(window->is_vsync_on(), tearing_supported);

		command_queue->signal(current_backbuffer_idx);

		current_backbuffer_idx = swap_chain->get_backbuffer_index();

		command_queue->wait_for_finished(current_backbuffer_idx);
	}
}

void CubeApplication::resize()
{
	const uint8_t n_backbuffers = 3;

	if (window->resize()) {
		command_queue->flush(swap_chain->get_backbuffer_index());

		for (uint8_t i = 0; i < n_backbuffers; ++i) {
			swap_chain->release_buffer(i);
			command_queue->reset_frame_fence_value(i, command_queue->get_fence_value());
		}

		swap_chain->resize_buffers(device, descriptor_heap, window->width(), window->height());
	}
}

}