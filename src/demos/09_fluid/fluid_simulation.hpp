#pragma once
#include "../../application.hpp"
#include "../../core/pso.hpp"
#include "../../core/vertex_buffer_resource.hpp"

namespace moonlight
{

class FluidSimulation : public IApplication
{

public:

    FluidSimulation(HINSTANCE);

    void resize() override;
    void render() override;
    void update() override;

private:

    void initialize_simulation();
    void initialize_d3d12_objects();

    void record_command_list();

private:

    VertexBufferResource m_vertex_buffer;
    std::unique_ptr<PipelineStateObject> m_pso_wrapper;
};

}