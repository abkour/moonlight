#include "gameplay_system.hpp"

#include <chrono>
#include <cstdio>

#include <Windows.h>

namespace moonlight {

GameplaySystem::GameplaySystem() 
	: frame_counter(0)
	, elapsed_seconds(0.f)
{
}

void GameplaySystem::update() {
	static auto t0 = std::chrono::high_resolution_clock::now();
	frame_counter++;
	auto t1 = std::chrono::high_resolution_clock::now();
	elapsed_seconds += (t1 - t0).count() * 1e-9;
	t0 = t1;
	if (elapsed_seconds > 1.f) {
		char buffer[500];
		auto fps = frame_counter / elapsed_seconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);

		frame_counter = 0;
		elapsed_seconds = 0.f;
	}
}

}