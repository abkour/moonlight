#pragma once
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <Windows.h>

namespace moonlight
{

std::size_t Align(std::size_t uLocation, std::size_t uAlign);

void compute_delta_time(float& elapsed_time);

}