#pragma once
#include <bitset>

namespace moonlight
{

struct KeyState
{
    void reset(const uint16_t i)
    {
        keys.reset(i);
    }

    void set(const uint16_t i)
    {
        keys.set(i);
    }

    bool operator[](const uint16_t i) const
    {
        return keys[i];
    }

    std::bitset<256> keys;
};

}