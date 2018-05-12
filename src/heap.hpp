#pragma once

#include "voxels.hpp"

namespace voxels {
	uint8_t* init();

	uint32_t allocate(uint32_t parent);

	void process();
}