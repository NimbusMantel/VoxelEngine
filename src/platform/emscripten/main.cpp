#include <stdio.h>
#include <functional>

#include <SDL2/SDL.h>
#include <emscripten/emscripten.h>

#include "game.h"

#include "platform_log.h"

void update();
void input();

const int width = 640, height = 360;

static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* screen;

static Uint32 buffer[width * height];
static Uint8 mask[width * height];

Uint32 currFrame;
Uint32 prevFrame;

bool isDragging = false;

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);

	screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	prevFrame = SDL_GetTicks();

	on_init(width, height, buffer, mask);

	emscripten_set_main_loop(update, 0, 1);

	SDL_Quit();

	return 0;
}

void update() {
	currFrame = SDL_GetTicks();

	input();
	on_update((currFrame - prevFrame) / 1000.0f);

	prevFrame = currFrame;

	SDL_UpdateTexture(screen, NULL, buffer, width * sizeof(Uint32));

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void input() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN: if (event.button.button == SDL_BUTTON_LEFT) { isDragging = true;  on_touch_down(event.button.x, height - event.button.y - 1); } break;
		case SDL_MOUSEBUTTONUP: if (event.button.button == SDL_BUTTON_LEFT) { isDragging = false; on_touch_up(event.button.x, height - event.button.y - 1); } break;
		case SDL_MOUSEMOTION: if (isDragging) { on_touch_move(event.motion.x, height - event.motion.y - 1, event.motion.xrel, -event.motion.yrel); } break;
		case SDL_MOUSEWHEEL: on_mouse_scroll(event.wheel.x, event.wheel.y); break;
		}
	}
}

/*
precision mediump float;
uniform sampler2D u_Texture;
varying vec2 v_Coordinate;

void main() {
	vec4 hwba = texture2D(u_TextureUnit, v_TextureCoordinates);

	hwba.x /= 60.0f;

	vec4 rgba = vce4(hueTOrgb(hwb.x + 2.0f), hueTOrgb(hwb.x), hueTOrgb(hwb.x - 2.0f), 255);

	rgba.x = (rgb.x * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgba.y = (rgb.y * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;
	rgba.z = (rgb.z * (1.0f - hwb.y - hwb.z) + hwb.y) * 255.0f;

	gl_FragColor = rgba;
}

float hueTOrgb(float hue) {
	if (hue < 0.0f) hue += 6.0f;
	else if (hue >= 6.0f) hue -= 6.0f;

	if (hue < 1.0f) return hue;
	else if (hue < 3) return 1.0f;
	else if (hue < 4) return (4.0f - hue);
	else return 0.0f;
}
*/