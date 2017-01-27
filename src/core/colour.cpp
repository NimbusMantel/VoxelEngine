#include "colour.h"

#include "platform_log.h"

#include <math.h>

uint32_t colourMix(uint32_t colA, uint32_t colB) {
	uint32_t rR, rG, rB; // Alpha value not considered

	rR = (((colA & 0xFF000000) >> 24) + ((colB & 0xFF000000) >> 24)) / 2;
	rG = (((colA & 0x00FF0000) >> 16) + ((colB & 0x00FF0000) >> 16)) / 2;
	rB = (((colA & 0x0000FF00) >> 8) + ((colB & 0x0000FF00) >> 8)) / 2;
	
	return ((rR << 24) | (rG << 16) | (rB << 8) | 255);
}