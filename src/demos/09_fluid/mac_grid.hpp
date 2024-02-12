#pragma once
#include <memory>

namespace moonlight
{

class MACGrid
{
public:

    MACGrid(unsigned dimx, unsigned dimy);

    float pressure_at(unsigned i, unsigned j) const;

private:

    unsigned m_dimx;
    unsigned m_dimy;

    std::unique_ptr<float[]> m_pressure;
    std::unique_ptr<float[]> m_velx;
    std::unique_ptr<float[]> m_vely;
};

}