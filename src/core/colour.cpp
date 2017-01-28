#include "colour.h"

#include "platform_log.h"

#include <math.h>

uint32_t colourMix(uint32_t colA, uint32_t colB) {
	float colA_r = ((colA & 0xFF000000) >> 24) / 255.0f;
	float colA_g = ((colA & 0x00FF0000) >> 16) / 255.0f;
	float colA_b = ((colA & 0x0000FF00) >> 8) / 255.0f;
	float colA_a = (colA & 0x000000FF) / 255.0f;

	float colB_r = ((colB & 0xFF000000) >> 24) / 255.0f;
	float colB_g = ((colB & 0x00FF0000) >> 16) / 255.0f;
	float colB_b = ((colB & 0x0000FF00) >> 8) / 255.0f;
	float colB_a = (colB & 0x000000FF) / 255.0f;

	if ((colA_a + colB_a) == 0.0f) return 0x00000000;

	float colR_r = colA_r + (colB_r - colA_r) * colB_a / (colA_a + colB_a);
	float colR_g = colA_g + (colB_g - colA_g) * colB_a / (colA_a + colB_a);
	float colR_b = colA_b + (colB_b - colA_b) * colB_a / (colA_a + colB_a);
	float colR_a = 1.0f - (1.0f - colA_a) * (1.0f - colB_a);
	
	return (((uint32_t)roundf(colR_r * 255.0f) << 24) | ((uint32_t)roundf(colR_g * 255.0f) << 16) | ((uint32_t)roundf(colR_b * 255.0f) << 8) | (uint32_t)roundf(colR_a * 255.0f));
}

double calcAbsorbance(double value) {
	if (value == 0) value = 0.00001;

	return (pow(1.0 - value, 2.0) / (2.0 * value));
}

double calcReflectance(double value) {
	return (1.0 + value - sqrt(pow(value, 2.0) + 2.0 * value));
}