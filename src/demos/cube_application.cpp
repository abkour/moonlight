#include "cube_application.hpp"

namespace moonlight {

CubeApplication::CubeApplication(HINSTANCE hinstance)
{
	const wchar_t* window_class_name = L"D3D12 Learning Application";
	const wchar_t* window_title = L"D3D12CubeWindow";
	uint32_t width = 1024;
	uint32_t height = 720;

	window = std::make_unique<Window>(
		hinstance,
		window_class_name,
		window_title,
		width,
		height,
		&CubeApplication::WindowMessagingProcess,
		this
	);

	rendering_system = std::make_unique<RenderingSystem>(
		window,
		3
	);

	gameplay_system = std::make_unique<GameplaySystem>();
}


}