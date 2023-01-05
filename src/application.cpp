#include "application.hpp"

namespace moonlight {

Application::Application(HINSTANCE hinstance) 
{
	const wchar_t* window_class_name = L"D3D12 Learning Application";
	const wchar_t* window_title = L"D3D12Window";
	uint32_t width = 1024;
	uint32_t height = 720;

	window = std::make_unique<Window>(
		hinstance, 
		window_class_name, 
		window_title, 
		width, 
		height, 
		&Application::WindowMessagingProcess, 
		this);

	rendering_system = std::make_unique<RenderingSystem>(
		window,
		3
	);

	gameplay_system = std::make_unique<GameplaySystem>();
}

void Application::run() {
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
	rendering_system->flush();
}

LRESULT CALLBACK Application::WindowMessagingProcess(
	HWND hwnd, 
	UINT message, 
	WPARAM wParam, 
	LPARAM lParam) 
{
	Application* app = nullptr;
	if (message == WM_NCCREATE) {
		// Before the window is created, we store the this pointer in the 
		// user_data field associated with the window.
		app = static_cast<Application*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		SetLastError(0);	// ???
		if (!SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app))) {
			if (GetLastError() != 0) {
				return FALSE;
			}
		}
	}

	// Retrieve the pointer. app is nullptr, when WM_NCCREATE has not yet been handled.
	app = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (app == nullptr) {
		return FALSE;
	}
	
	if (app->rs_is_initialized()) {
		switch (message) {
		case WM_PAINT:
			app->gameplay_system->update();
			app->rendering_system->render(app->window);
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