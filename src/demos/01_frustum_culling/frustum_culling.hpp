#pragma once 
#include "../../application.hpp"
#include "../../camera.hpp"
#include "../../simple_math.hpp"
#include "../../dx12_resource.hpp"
#include "../../render_texture.hpp"

namespace moonlight
{

class FrustumCulling : public IApplication
{

public:

	FrustumCulling(HINSTANCE hinstance);

	bool is_application_initialized() override;

	void flush() override;
	void on_key_down(WPARAM wparam) override;
	void on_mouse_move(LPARAM lparam) override;
	void render() override;
	void resize() override;
	void update() override;

private:

	Microsoft::WRL::ComPtr<ID3D12Device2> device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swap_chain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> quad_rtv_descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsv_descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap;
	Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> depth_buffer2;
	Microsoft::WRL::ComPtr<ID3D12Resource> backbuffers[3];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	Microsoft::WRL::ComPtr<ID3D12CommandList> command_list_copy;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list_direct;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> scene_root_signature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> scene_pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> quad_root_signature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> quad_pso;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> quad_vertex_buffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> instance_id_buffer;
	D3D12_VERTEX_BUFFER_VIEW instance_id_buffer_view;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> instance_descriptor_heap;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
	std::unique_ptr<DX12Resource> instance_buffer;

	uint64_t fence_value;
	HANDLE fence_event;
	D3D12_VIEWPORT viewport0;
	D3D12_VIEWPORT viewport1;
	D3D12_VIEWPORT viewport2;
	D3D12_RECT scissor_rect;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
	D3D12_VERTEX_BUFFER_VIEW quad_vertex_buffer_view;

	DirectX::XMMATRIX mvp_matrix;
	DirectX::XMMATRIX mvp_matrix_v2;
	bool app_initialized;

	void load_assets();
	void load_scene_shader_assets();
	void load_quad_shader_assets();

	void transition_resource(
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES before_state,
		D3D12_RESOURCE_STATES after_state
	);

	void command_queue_signal(uint64_t fence_value);
	void flush_command_queue();
	void wait_for_fence(uint64_t fence_value);

	std::unique_ptr<RenderTexture> scene_texture;
	std::unique_ptr<RenderTexture> ortho_scene_texture;

private:

	// Mouse/Keyboard controls
	uint16_t window_width, window_height;
	Camera camera;
	Camera top_down_camera;

	uint32_t xcoord_old;
	uint32_t ycoord_old;
	bool first_cursor_entry = true;

	float elapsed_time = 0.f;
};

}