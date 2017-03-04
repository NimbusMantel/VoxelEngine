#include "colour.h"

#include "platform_log.h"

#include <math.h>

static uint32_t lastColA = 0x00000000;
static uint32_t lastColB = 0x00000000;
static uint32_t lastWeig = 0x00;
static uint32_t lastColR = 0x00000000;
static int16_t alp;

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

uint32_t colourMix(uint32_t colA, uint32_t colB, uint8_t& weight) {
	weight |= !bool(weight);

	if (lastColA == colA && lastColB == colB && lastWeig == weight) {
		weight += (weight < 0xFE);

		return lastColR;
	}

	lastColA = colA;
	lastColB = colB;
	lastWeig = weight;

	alp = ((((colA & 0x003F8000) >> 15) == 0x7F) || (((colA & 0x00007F00) >> 8) == 0x7F));
	colA = (colA & 0x803FFFFF) | ((colB & 0x7FC00000) & -alp) | ((colA & 0x7FC00000) & ~(-alp));

	alp = ((((colB & 0x003F8000) >> 15) == 0x7F) || (((colB & 0x00007F00) >> 8) == 0x7F));
	colB = (colB & 0x803FFFFF) | ((colA & 0x7FC00000) & -alp) | ((colB & 0x7FC00000) & ~(-alp));

	alp = ((colA & 0x7FC00000) >> 22) - ((colB & 0x7FC00000) >> 22);
	alp = 0xB4 & -(((alp ^ (alp >> 15)) - (alp >> 15)) > 0xB4);
	lastColR = (((((colB & 0x7FC00000) >> 22) + 0x0168 + (int32_t)(((((colA & 0x7FC00000) >> 22) + alp) % 0x0168) - ((((colB & 0x7FC00000) >> 22) + alp) % 0x0168)) * (int32_t)weight * (int32_t)(colA & 0xFF) / (int32_t)(weight * (colA & 0xFF) + (colB & 0xFF))) % 0x0168) << 22) & 0x7FC00000;

	alp = 0x7F * weight * (colA & 0xFF) / (weight * (colA & 0xFF) + (colB & 0xFF));
	lastColR |= (((((colA & 0x003F8000) >> 15) * (alp + 1) + ((colB & 0x003F8000) >> 15) * (0x7F - alp)) << 8) & 0x003F8000) |
		(((((colA & 0x00007F00) >> 8) * (alp + 1) + ((colB & 0x00007F00) >> 8) * (0x7F - alp)) << 1) & 0x00007F00) |
		((0xFF00 - ((0xFF - (colA & 0x000000FF)) * (0xFF - (colB & 0x000000FF)))) >> 8);

	weight += (weight < 0xFE);

	return lastColR;
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