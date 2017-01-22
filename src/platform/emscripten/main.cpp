#include <stdio.h>
#include <functional>

#include <emscripten/emscripten.h>
#include <SDL/SDL.h>

#include "game.h"

#include "platform_log.h"

void update();
void input();

const int width = 640, height = 360;

static SDL_Surface* screen;
static Uint32* buffer;
static bool mask[width * height];

Uint32 currFrame;
Uint32 prevFrame;

bool isDragging = false;

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
	buffer = (Uint32*)screen->pixels;
	SDL_PixelFormat* format = screen->format;

	prevFrame = SDL_GetTicks();

	on_init(width, height, buffer, mask, [format](Uint32 rgba) -> Uint32 {return SDL_MapRGBA(format, (rgba & 0xFF000000) >> 24, (rgba & 0x00FF0000) >> 16, (rgba & 0x0000FF00) >> 8, rgba & 0x000000FF); });

	emscripten_set_main_loop(update, 0, 1);

	SDL_Quit();

	return 0;
}

void update() {
	if (SDL_MUSTLOCK(screen)) {
		SDL_LockSurface(screen);
	}

	currFrame = SDL_GetTicks();

	input();
	on_update((currFrame - prevFrame) / 1000.0f);

	prevFrame = currFrame;

	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}

	SDL_Flip(screen);
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