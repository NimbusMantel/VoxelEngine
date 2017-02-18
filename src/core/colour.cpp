#include "colour.h"

#include "platform_log.h"

#include <math.h>
#include <assert.h>

// TO DO: Optimisation

uint32_t rgbaTOhwba(uint32_t rgba) {
	if (!(rgba & 0x000000FF)) return 0x00000000;
	
	col hwb = rgbTOhwb(col((rgba & 0xFF000000) >> 24, (rgba & 0x00FF0000) >> 16, (rgba & 0x0000FF00) >> 8));

	return (((uint32_t)roundf(hwb.x) << 22) | ((uint32_t)roundf(hwb.y * 127.0f) << 15) | ((uint32_t)roundf(hwb.z * 127.0f) << 8) | (rgba & 0x000000FF));
}

uint32_t hwbaTOrgba(uint32_t hwba) {
	if (!(hwba & 0x000000FF)) return 0x00000000;

	col rgb = hwbTOrgb(col((hwba & 0x7FC00000) >> 22, ((hwba & 0x003F8000) >> 15) / 127.0f, ((hwba & 0x00007F00) >> 8) / 127.0f));

	return (((uint32_t)roundf(rgb.x) << 24) | ((uint32_t)roundf(rgb.y) << 16) | ((uint32_t)roundf(rgb.z) << 8) | (hwba & 0x000000FF));
}

uint32_t colourMix(uint32_t colA, uint32_t colB) {
	float alpA = (colA & 0x000000FF) / 255.0f;
	float alpB = (colB & 0x000000FF) / 255.0f;

	if (alpA < M_EPSILON && alpB > M_EPSILON) return colB;
	if (alpB < M_EPSILON && alpA > M_EPSILON) return colA;
	if (alpA < M_EPSILON && alpB < M_EPSILON) return 0x0;

	col hwbA = col((colA & 0x7FC00000) >> 22, ((colA & 0x003F8000) >> 15) / 127.0f, ((colA & 0x00007F00) >> 8) / 127.0f);
	col hwbB = col((colB & 0x7FC00000) >> 22, ((colB & 0x003F8000) >> 15) / 127.0f, ((colB & 0x00007F00) >> 8) / 127.0f);

	if (hwbA.y == 1.0f || hwbA.z == 1.0f) hwbA.x = hwbB.x;
	else if (hwbB.y == 1.0f || hwbB.z == 1.0f) hwbB.x = hwbA.x;
	else if ((hwbA.x - hwbB.x) > 180.0f) hwbB.x += 360.0f;
	else if ((hwbA.x - hwbB.x) < -180.0f) hwbA.x += 360.0f;

	float h = fmodf(hwbA.x + (hwbB.x - hwbA.x) * alpB / (alpA + alpB), 360.0f);
	float w = hwbA.y + (hwbB.y - hwbA.y) * alpB / (alpA + alpB);
	float b = hwbA.z + (hwbB.z - hwbA.z) * alpB / (alpA + alpB);

	return (((uint32_t)roundf(h) << 22) | ((uint32_t)roundf(w * 127.0f) << 15) | ((uint32_t)roundf(b * 127.0f) << 8) | (uint32_t)roundf((1.0f - (1.0f - alpA) * (1.0f - alpB)) * 255.0f));
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

	col rgb(hueTOrgb(hwb.x + 2.0f), hueTOrgb(hwb.x), hueTOrgb(hwb.x - 2.0f));

	rgb.x = (rgb.x * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgb.y = (rgb.y * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgb.z = (rgb.z * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;

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