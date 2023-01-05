#pragma once
#include "gameplay_system.hpp"
#include "rendering_system.hpp"
#include "window.hpp"

#include <memory>

namespace moonlight {

class Application {

public:

	Application(HINSTANCE hinstance);

	void update() {
		gameplay_system->update();
	}
	
	void render() {
		rendering_system->render(window);
	}

	void run();

	bool rs_is_initialized() {
		if (rendering_system == nullptr) {
			return false;
		}
		return rendering_system->is_initialized();
	}

private:

	static LRESULT CALLBACK WindowMessagingProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

	std::unique_ptr<GameplaySystem> gameplay_system;
	std::unique_ptr<RenderingSystem> rendering_system;
	std::unique_ptr<Window> window;
};

}