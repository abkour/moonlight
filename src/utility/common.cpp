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

}