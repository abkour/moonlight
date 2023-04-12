#pragma once
#include <bitset>

namespace moonlight
{

enum KeyCode
{
    Reserved = 0x00,
    Tab = 0x09,
    Enter = 0x0D,
    Shift = 0x10,
    Control = 0x11,
    Alt = 0x12,
    Spacebar = 0x20,
    N0 = 0x30,
    N1 = 0x31,
    N2 = 0x32,
    N3 = 0x33,
    N4 = 0x34,
    N5 = 0x35,
    N6 = 0x36,
    N7 = 0x37,
    N8 = 0x38,
    N9 = 0x39,
    A = 0x41,
    B = 0x42,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    F = 0x46,
    G = 0x47,
    H = 0x48,
    I = 0x49,
    J = 0x4A,
    K = 0x4B,
    L = 0x4C,
    M = 0x4D,
    N = 0x4E,
    O = 0x4F,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    U = 0x55,
    V = 0x56,
    W = 0x57,
    X = 0x58,
    Y = 0x59,
    Z = 0x5A,
    F1 = 0x70,
    F2 = 0x71,
    F3 = 0x72,
    F4 = 0x73,
    F5 = 0x74,
    F6 = 0x75,
    F7 = 0x76,
    F8 = 0x77,
    F9 = 0x78,
    F10 = 0x79,
    F11 = 0x7A,
    F12 = 0x7B,
    F13 = 0x7C,
    F14 = 0x7D,
    F15 = 0x7E,
    F16 = 0x7F,
    F17 = 0x80,
    F18 = 0x81,
    F19 = 0x82,
    F20 = 0x83,
    F21 = 0x84,
    F22 = 0x85,
    F23 = 0x86,
    F24 = 0x87,
    LShift = 0xA0,
    RShift = 0xA1,
    LCtrl = 0xA2,
    RCtrl = 0XA3,
    LAlt = 0xA4,
    RAlt = 0xA5
};

struct PackedKeyArguments
{
    enum KeyState
    {
        Released = 0,
        Pressed
    };

    PackedKeyArguments(KeyCode key, KeyCode sys_key, KeyState key_state)
        : key(key)
        , sys_key(sys_key)
        , key_state(key_state)
    {}

    KeyCode key;
    KeyCode sys_key;
    KeyState key_state;
};

struct KeyState
{
    void reset(const uint16_t i)
    {
        keys.reset(i);
        char x = 'A';
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