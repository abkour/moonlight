#pragma once
#include "../application.hpp"
#include "../dx12_wrapper.hpp"

namespace moonlight {

class CubeApplication : public IApplication {

public:

	CubeApplication(HINSTANCE hinstance);

	~CubeApplication() {}

	void flush() override;

	void update() override {
		gameplay_system->update();
	}

	void render() override;

	void resize() override;

	virtual bool is_application_initialized() {
		return system_initialized;
	}

private:

	bool system_initialized;

private:

	Microsoft::WRL::ComPtr<ID3D12Device2> device;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocators[3];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;

	std::unique_ptr<CommandQueue> command_queue;
	std::unique_ptr<SwapChain> swap_chain;
};

}