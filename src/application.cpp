#include "application.hpp"

using namespace Microsoft::WRL;

namespace moonlight {

void IApplication::run() {
	::ShowWindow(window->handle, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	// Make sure the command queue has finished all commands before closing.
	flush();
}

LRESULT CALLBACK IApplication::WindowMessagingProcess(
	HWND hwnd, 
	UINT message, 
	WPARAM wParam, 
	LPARAM lParam) 
{
	IApplication* app = nullptr;
	if (message == WM_NCCREATE) {
		// Before the window is created, we store the this pointer in the 
		// user_data field associated with the window.
		app = static_cast<IApplication*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		SetLastError(0);	// ???
		if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app))) {
			if (GetLastError() != 0) {
				return FALSE;
			}
		}
	}

	// Retrieve the pointer. app is nullptr, when WM_NCCREATE has not yet been handled.
	app = reinterpret_cast<IApplication*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (app == nullptr) {
		return FALSE;
	}
	
	if (app->is_application_initialized()) {
		switch (message) {
		case WM_PAINT:
			app->gameplay_system->update();
			app->render();
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			switch (wParam) {
			case 'V':
				app->window->flip_vsync();
				break;
			case VK_ESCAPE:
				::PostQuitMessage(0);
				break;
			case VK_F11:
				app->window->flip_fullscreen();	// broken
				break;
			}
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
			app->window->resize();
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	} else { 
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

}