#include "mouse.hpp"

namespace moonlight
{

MouseInterface::MouseInterface()
    : m_deltax(0), m_deltay(0)
{
    POINT cursor_pos;
    GetCursorPos(&cursor_pos);

    m_posx = cursor_pos.x;
    m_posy = cursor_pos.y;
}

void MouseInterface::on_mouse_event(LPARAM lparam)
{
    UINT size;

    GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[size];
    if (lpb == NULL) return;

    if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER)) != size)
    {
        OutputDebugStringA("GetRawInputData does not report correct size");
    }
    RAWINPUT* raw = (RAWINPUT*)lpb;
    if (raw->header.dwType == RIM_TYPEMOUSE)
    {
        switch (raw->data.mouse.usButtonFlags)
        {
        case RI_MOUSE_BUTTON_1_DOWN:
            m_last_event = LMB_Pressed;
            m_lmb_state = Down;
            break;
        case RI_MOUSE_BUTTON_1_UP:
            m_last_event = LMB_Released;
            m_lmb_state = Up;
            break;
        case RI_MOUSE_BUTTON_2_DOWN:
            m_last_event = RMB_Pressed;
            m_rmb_state = Down;
            break;
        case RI_MOUSE_BUTTON_2_UP:
            m_last_event = RMB_Released;
            m_rmb_state = Up;
            break;
        case RI_MOUSE_BUTTON_3_DOWN:
            m_last_event = Wheel_Pressed;
            m_wheel_state = Down;
            break;
        case RI_MOUSE_BUTTON_3_UP:
            m_last_event = Wheel_Released;
            m_wheel_state = Up;
            break;
        case RI_MOUSE_WHEEL:
            m_wheel_delta = raw->data.mouse.usButtonData;
            break;
        default:
            break;
        }

        switch(raw->data.mouse.usFlags)
        { 
        case MOUSE_MOVE_RELATIVE:
            m_posx = static_cast<INT>(m_posx) + raw->data.mouse.lLastX;
            m_posy = static_cast<INT>(m_posy) + raw->data.mouse.lLastY;
            m_deltax = raw->data.mouse.lLastX;
            m_deltay = raw->data.mouse.lLastY;
            break;
        case MOUSE_MOVE_ABSOLUTE:
            m_deltax = raw->data.mouse.lLastX - m_posx;
            m_deltay = raw->data.mouse.lLastY - m_posy;
            m_posx = raw->data.mouse.lLastX;
            m_posy = raw->data.mouse.lLastY;
            break;
        default:
            break;
        }
    }

    delete[] lpb;
}

}