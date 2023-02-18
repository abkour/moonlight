#pragma once
#include <DirectXMath.h>

#include <memory>

namespace moonlight
{

struct InstanceAttributes
{
    DirectX::XMFLOAT4 displacement;
    DirectX::XMFLOAT4 color;
};

inline void construct_scene_of_cubes(
    InstanceAttributes* instance_array,
    const float xdim,
    const float zdim,
    const float cubes_per_row,
    const float cubes_per_column)
{
    const float xdelta = xdim / cubes_per_row;
    const float zdelta = zdim / cubes_per_column;
    float xpos = -xdim / 2.f;
    for (int x = 0; x < cubes_per_column; ++x)
    {
        float zpos = -zdim / 2.f;
        for (int z = 0; z < cubes_per_row; ++z)
        {
            int idx = x * cubes_per_column + z;
            instance_array[idx].displacement =
                DirectX::XMFLOAT4(xpos, 0.f, zpos, 0.f);
            instance_array[idx].color =
                DirectX::XMFLOAT4(0.2f, 1.f, 1.f, 1.f);

            zpos += zdelta;
        }

        xpos += xdelta;
    }
}

}