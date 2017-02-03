#include "colour.h"

#include "platform_log.h"

#include <math.h>
#include <assert.h>

// TO DO: Optimisation

uint32_t colourMix(uint32_t colA, uint32_t colB) {
	float alpA = (colA & 0x000000FF) / 255.0f;
	float alpB = (colB & 0x000000FF) / 255.0f;

	if (alpA == 0.0f && alpB > 0.0f) return colB;
	if (alpB == 0.0f && alpA > 0.0f) return colA;
	if (alpA == 0.0f && alpB == 0.0f) return 0x0;

	col hwbA = rgbTOhwb(col((colA & 0xFF000000) >> 24, (colA & 0x00FF0000) >> 16, (colA & 0x0000FF00) >> 8));
	col hwbB = rgbTOhwb(col((colB & 0xFF000000) >> 24, (colB & 0x00FF0000) >> 16, (colB & 0x0000FF00) >> 8));

	if (hwbA.y == 1.0f || hwbA.z == 1.0f) hwbA.x = hwbB.x;
	else if (hwbB.y == 1.0f || hwbB.z == 1.0f) hwbB.x = hwbA.x;
	else if ((hwbA.x - hwbB.x) > 180.0f) hwbB.x += 360.0f;
	else if ((hwbA.x - hwbB.x) < -180.0f) hwbA.x += 360.0f;

	float h = fmodf(hwbA.x + (hwbB.x - hwbA.x) * alpB / (alpA + alpB), 360.0f);
	float w = hwbA.y + (hwbB.y - hwbA.y) * alpB / (alpA + alpB);
	float b = hwbA.z + (hwbB.z - hwbA.z) * alpB / (alpA + alpB);

	col rgb = hwbTOrgb(col(h, w, b));

	return (((uint32_t)roundf(rgb.x) << 24) | ((uint32_t)roundf(rgb.y) << 16) | ((uint32_t)roundf(rgb.z) << 8) | (uint32_t)roundf((1.0f - (1.0f - alpA) * (1.0f - alpB)) * 255.0f));
}

col rgbTOhwb(col rgb) {
	rgb.x /= 255.0f;
	rgb.y /= 255.0f;
	rgb.z /= 255.0f;

	float max = MAX(rgb.x, MAX(rgb.y, rgb.z));
	float min = MIN(rgb.x, MIN(rgb.y, rgb.z));
	float chr = max - min;

	col hwb;

	if (chr == 0.0f) {
		hwb.x = 0.0f;
	}
	else if (rgb.x == max) {
		hwb.x = (rgb.y - rgb.z) / chr;
	}
	else if (rgb.y == max) {
		hwb.x = (rgb.z - rgb.x) / chr + 2.0f;
	}
	else {
		hwb.x = (rgb.x - rgb.y) / chr + 4.0f;
	}

	hwb.x = fmodf(hwb.x * 60.0f + 360.0f, 360.0f);
	hwb.y = min;
	hwb.z = 1.0f - max;

	return hwb;
}

col hwbTOrgb(col hwb) {
	hwb.x /= 60.0f;

	col rgb(hueTOrgb(hwb.x + 2.0f) * 255.0f, hueTOrgb(hwb.x) * 255.0f, hueTOrgb(hwb.x - 2.0f) * 255.0f);

	rgb.x = ((rgb.x / 255.0f) * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgb.y = ((rgb.y / 255.0f) * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgb.z = ((rgb.z / 255.0f) * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;

	return rgb;
}

float hueTOrgb(float hue) {
	if (hue < 0.0f) hue += 6.0f;
	else if (hue >= 6.0f) hue -= 6.0f;

	if (hue < 1.0f) return hue;
	else if (hue < 3) return 1.0f;
	else if (hue < 4) return (4.0f - hue);
	else return 0.0f;
}