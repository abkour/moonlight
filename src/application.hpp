#pragma once
#include "gameplay_system.hpp"
#include "window.hpp"

#include <memory>

namespace moonlight {

class IApplication {

public:

	IApplication() {}
	IApplication(HINSTANCE hinstance);

	virtual ~IApplication() = 0 {}

	virtual bool is_application_initialized() = 0;

	virtual void flush() = 0;

	virtual void update() = 0;
	
	virtual void render() = 0;

	virtual void resize() = 0;

	void run();

	static LRESULT CALLBACK WindowMessagingProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:

	//std::unique_ptr<GameplaySystem> gameplay_system;
	std::unique_ptr<Window> window;
};

}