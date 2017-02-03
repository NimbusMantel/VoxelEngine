#pragma once

#include "macros.h"

#include <cstdint>

struct col {
	col(float cx, float cy, float cz) : x(cx), y(cy), z(cz) {};
	col() : col(0.0f, 0.0f, 0.0f) {};
	
	float x, y, z;
};

uint32_t colourMix(uint32_t colA, uint32_t colB);

col rgbTOhwb(col rgb);
col hwbTOrgb(col hwb);

float hueTOrgb(float hue);