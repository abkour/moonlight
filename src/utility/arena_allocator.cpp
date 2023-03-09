#include "arena_allocator.hpp"

namespace moonlight
{

ArenaAllocator::ArenaAllocator(std::size_t n_bytes)
{
    m_data = reinterpret_cast<void*>(new char[n_bytes]);
    m_current = m_data;
    m_available_bytes = n_bytes;
}

ArenaAllocator::~ArenaAllocator()
{
    delete[] reinterpret_cast<char*>(m_data);
}

void* ArenaAllocator::allocate(std::size_t n_bytes)
{
    return _pimpl_allocate(n_bytes, 0);
}

void* ArenaAllocator::allocate_aligned(std::size_t n_bytes, std::size_t align)
{
    --align;
    std::size_t aligned = (n_bytes + align) & (~align);
    std::size_t align_diff = aligned - n_bytes;

    return _pimpl_allocate(n_bytes, align_diff);
}

void* ArenaAllocator::_pimpl_allocate(std::size_t n_bytes, std::size_t align_diff)
{
    n_bytes += align_diff;
    if (n_bytes <= m_available_bytes)
    {
        void* new_block = static_cast<char*>(m_current) + align_diff;
        m_current = static_cast<char*>(m_current) + n_bytes;
        m_available_bytes -= n_bytes;
        return new_block;
    }
    else
    {
        return nullptr;
    }
}

}