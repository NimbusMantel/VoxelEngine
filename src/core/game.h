#pragma once

#include <cstdint>
#include <functional>

void on_init(int width, int height, uint32_t* buffer, uint8_t* mask, std::function<uint32_t(uint32_t)> toPixel, std::function<uint32_t(uint32_t)> fromPixel);

void on_update(float dt);

void on_touch_down(int x, int y);
void on_touch_move(int x, int y, int dx, int dy);
void on_touch_up(int x, int y);

void on_mouse_scroll(int x, int y);