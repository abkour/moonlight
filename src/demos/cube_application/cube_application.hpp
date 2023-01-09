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

	void flush() override;

	void update() override;

	void render() override;

	void resize() override;

	bool is_application_initialized() override {
		return system_initialized && content_loaded;
	}

public:
	
	void resize_depth_buffer(int width, int height);
	void resize_window(int width, int height);

	void clear_depth(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, 
		FLOAT depth = 1.f
	);

	void clear_rtv(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv,
		FLOAT* clear_color
	);

	void transition_resource(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES before_state,
		D3D12_RESOURCE_STATES after_state
	);

private:

	bool content_loaded;
	bool system_initialized;

	uint64_t fence_values[3];

private:

	Microsoft::WRL::ComPtr<ID3D12Device2> device;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptor_heap;

	std::unique_ptr<CommandQueue> command_queue_compute;
	std::unique_ptr<CommandQueue> command_queue_copy;
	std::unique_ptr<CommandQueue> command_queue_direct;
	std::unique_ptr<SwapChain> swap_chain;

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

	Microsoft::WRL::ComPtr<ID3D12Resource> index_buffer;
	D3D12_INDEX_BUFFER_VIEW index_buffer_view;

	Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_heap;	// depth/stencil dheap

	Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissor_rect;

	float fov;

	DirectX::XMMATRIX model_matrix;
	DirectX::XMMATRIX view_matrix;
	DirectX::XMMATRIX projection_matrix;

private:

	// Helper functions
	void load_content();

	void update_buffer_resource(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> command_list,
		ID3D12Resource** destination_resource,
		ID3D12Resource** intermediate_resource,
		size_t num_elements,
		size_t element_size,
		const void* buffer_data,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
	);
};

}