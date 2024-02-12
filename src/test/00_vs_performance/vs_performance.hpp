#pragma once
#include "../../application.hpp"
#include "../../core/pso.hpp"
#include "../../core/vertex_buffer_resource.hpp"


namespace moonlight
{

class VSPerformance : public IApplication
{
public:

    VSPerformance(HINSTANCE);

    void render() override;
    void resize() override;
    void update() override;

private:

    void initialize_state();
    void record_command_list(ID3D12GraphicsCommandList* command_list);

private:

    VertexBufferResource m_vertex_buffer;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper;

};

}