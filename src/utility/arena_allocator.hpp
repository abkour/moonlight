#pragma once
#include <cstddef>

namespace moonlight
{

class ArenaAllocator
{
public:

    ArenaAllocator(std::size_t n_bytes = 0x3FFFF);
    ~ArenaAllocator();

    void* allocate(std::size_t n_bytes);

    void* allocate_aligned(std::size_t n_bytes, std::size_t align);

private:

    void* _pimpl_allocate(std::size_t n_bytes, std::size_t align_diff);

    void* data;
    void* current;
    std::size_t available_bytes;
};

}