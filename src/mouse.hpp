#pragma once
#include <Windows.h>

namespace moonlight
{

class MouseInterface
{
public:

    enum MouseEvents
    {
        NONE = 0,
        LMB_Pressed,
        LMB_Released,
        RMB_Pressed,
        RMB_Released,
        Wheel_Pressed,
        Wheel_Released,
        Wheel_Scroll
    };

    enum MouseButtonState
    {
        Up = 0, 
        Down = 1
    };

public:

    MouseInterface();

    void on_mouse_event(LPARAM lparam);

    INT deltax() const { return m_deltax; }
    INT deltay() const { return m_deltay; }
    UINT posx() const { return m_posx; }
    UINT posy() const { return m_posy; }

    MouseButtonState lmb_state() const { return m_lmb_state; }
    MouseButtonState rmb_state() const { return m_rmb_state; }
    MouseButtonState wheel_state() const { return m_wheel_state; }
    INT wheel_delta() const { return m_wheel_delta; }

    MouseEvents last_event() const { return m_last_event; }

private:

    INT m_deltax, m_deltay;
    UINT m_posx, m_posy;
    UINT m_wheel_delta;
    MouseButtonState m_lmb_state;
    MouseButtonState m_rmb_state;
    MouseButtonState m_wheel_state;
    MouseEvents m_last_event;
};

}