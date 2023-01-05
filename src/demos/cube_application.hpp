#pragma once
#include "../application.hpp"

namespace moonlight {

class CubeApplication : public IApplication {

public:

	CubeApplication(HINSTANCE hinstance);

	~CubeApplication() {}

	void update() override {
		gameplay_system->update();
	}

	void render() override {
		rendering_system->render(window);
	}

	virtual bool is_application_initialized() {
		if (rendering_system == nullptr) {
			return false;
		}
		return rendering_system->is_initialized();
	}
};

}