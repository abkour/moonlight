#pragma once
#include <cstdint>

namespace moonlight {

class GameplaySystem {

public:

	GameplaySystem();

	void update();

private:

	float elapsed_seconds;
	uint64_t frame_counter;
};

}