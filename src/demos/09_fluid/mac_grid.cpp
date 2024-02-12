#pragma once
#include "mac_grid.hpp"

namespace moonlight
{

MACGrid::MACGrid(unsigned dimx, unsigned dimy)
    : m_dimx(dimx)
    , m_dimy(dimy)
{
    m_pressure = std::make_unique<float[]>(dimx * dimy);

    m_velx = std::make_unique<float[]>(dimx * (dimy + 1));
    m_vely = std::make_unique<float[]>((dimx + 1) * dimy);
}

float MACGrid::pressure_at(unsigned i, unsigned j) const
{
    uint64_t idx = i + j * m_dimx;

    return m_pressure[idx];
}

}