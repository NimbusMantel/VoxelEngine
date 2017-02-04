#pragma once

#include "macros.h"

#include <cstdint>

void on_init(int width, int height, uint32_t* buffer, uint8_t* mask);

void on_update(float dt);

void on_touch_down(int x, int y);
void on_touch_move(int x, int y, int dx, int dy);
void on_touch_up(int x, int y);

void on_mouse_scroll(int x, int y);