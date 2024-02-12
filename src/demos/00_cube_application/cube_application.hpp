#pragma once
#include "../../application.hpp"
#include "../../core/pso.hpp"
#include "../../core/vertex_buffer_resource.hpp"

namespace moonlight {

class CubeApplication : public IApplication {

public:

    CubeApplication(HINSTANCE hinstance);

    ~CubeApplication() {}

    void update() override;

    void render() override;

    void resize() override;

private:

    void load_assets();
    void record_command_list();

private:

    std::unique_ptr<PipelineStateObject> m_pso_wrapper;
    std::unique_ptr<VertexBufferResource> m_vertex_buffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srv_descriptor_heap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;

    void load_texture_from_file(
        const wchar_t* filename
    );

    DirectX::XMMATRIX m_mvp_matrix;
};

}