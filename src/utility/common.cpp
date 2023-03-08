#include "common.hpp"

namespace moonlight
{

std::size_t Align(std::size_t uLocation, std::size_t uAlign)
{
    if ((0 == uAlign) || (uAlign & (uAlign - 1)))
    {
        throw std::runtime_error("non-pow2 alignment");
    }

    return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
}

void compute_delta_time(float& elapsed_time)
{
    static float elapsed_time_at_threshold = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    static float total_time = 0.f;
    auto t1 = std::chrono::high_resolution_clock::now();
    elapsed_time = (t1 - t0).count() * 1e-9;
    total_time += elapsed_time;
    t0 = t1;
}

}