#include "game.h"

#include <stdlib.h>
#include <math.h>
#include <string>

#include "platform_log.h"
#include "platform_file_utils.h"

#include "voxel.h"
#include "camera.h"

#include "json.h"
using json = nlohmann::json;

static int width, height;
static uint32_t* buffer;
static bool* mask;
static std::function<uint32_t(uint32_t)> convert;

static VoxelBuffer voxels = VoxelBuffer();
static Camera camera;

static vec2 mouse_pos;

static double rotX = 0.0;
static double rotY = 0.0;

static double distance = 75.0;

void teapotTest() {
	json teapot = json::parse(get_file_data("teapot.json").data);

	std::string hex;
	uint32_t colour;

	for (json::iterator it = teapot["voxels"].begin(); it != teapot["voxels"].end(); ++it) {
		hex = (*it)["colour"].get<std::string>();
		colour = std::stoul(hex, nullptr, 16);
		
		voxels.addVoxel((*it)["x"], (*it)["y"], (*it)["z"], 1, VoxelBuffer::constructVoxel(colour));
	}
}

void monkeyTest() {
	json monkey = json::parse(get_file_data("monkey.json").data);

	std::string hex;
	uint32_t colour;

	for (json::iterator it = monkey["voxels"].begin(); it != monkey["voxels"].end(); ++it) {
		hex = (*it)["colour"].get<std::string>();
		colour = std::stoul(hex, nullptr, 16);

		voxels.addVoxel((*it)["x"], (*it)["y"], (*it)["z"], 1, VoxelBuffer::constructVoxel(colour));
	}
}

void axisTest() {
	for (uint8_t i = 0; i < 8; ++i) {
		voxels.addVoxel((i & 0x01) ? 1 : -1, (i & 0x02) ? 1 : -1, (i & 0x04) ? 1 : -1, 1, VoxelBuffer::constructVoxel(0xFFFFFFFF));
	}

	for (uint8_t i = 3; i < 17; i += 2) {
		voxels.addVoxel(i, 1, 1, 1, VoxelBuffer::constructVoxel(0xFF0000FF));
		voxels.addVoxel(i, -1, 1, 1, VoxelBuffer::constructVoxel(0xFF0000FF));
		voxels.addVoxel(i, 1, -1, 1, VoxelBuffer::constructVoxel(0xFF0000FF));
		voxels.addVoxel(i, -1, -1, 1, VoxelBuffer::constructVoxel(0xFF0000FF));
	}
	
	for (uint8_t i = 3; i < 17; i += 2) {
		voxels.addVoxel(1, i, 1, 1, VoxelBuffer::constructVoxel(0x00FF00FF));
		voxels.addVoxel(-1, i, 1, 1, VoxelBuffer::constructVoxel(0x00FF00FF));
		voxels.addVoxel(1, i, -1, 1, VoxelBuffer::constructVoxel(0x00FF00FF));
		voxels.addVoxel(-1, i, -1, 1, VoxelBuffer::constructVoxel(0x00FF00FF));
	}

	for (uint8_t i = 3; i < 17; i += 2) {
		voxels.addVoxel(1, 1, i, 1, VoxelBuffer::constructVoxel(0x0000FFFF));
		voxels.addVoxel(-1, 1, i, 1, VoxelBuffer::constructVoxel(0x0000FFFF));
		voxels.addVoxel(1, -1, i, 1, VoxelBuffer::constructVoxel(0x0000FFFF));
		voxels.addVoxel(-1, -1, i, 1, VoxelBuffer::constructVoxel(0x0000FFFF));
	}
}

void on_init(int w, int h, uint32_t* b, bool* m, std::function<uint32_t(uint32_t)> c) {
	width = w;
	height = h;
	buffer = b;
	mask = m;
	convert = c;

	camera = Camera(voxels.getRenderFunction(width, height, 70, buffer, mask, convert));

	monkeyTest();
}

void on_update(float dt) {
	camera.setPositionX(sin(rotY) * distance);
	camera.setPositionY(sin(rotX) * cos(rotY) * distance);
	camera.setPositionZ(cos(rotX) * cos(rotY) * distance);

	camera.setRotationX(-rotX);
	camera.setRotationY(rotY);

	camera.render();
}

void on_touch_down(int x, int y) {
	mouse_pos.x = x;
	mouse_pos.y = y;
}

void on_touch_move(int x, int y, int dx, int dy) {
	rotY += 1.0 - pow(3.0, dx / (float)width);
	rotX += 1.0 - pow(3.0, dy / (float)height);

	mouse_pos.x = x;
	mouse_pos.y = y;
}

void on_touch_up(int x, int y) {
	mouse_pos.x = x;
	mouse_pos.y = y;
}

void on_mouse_scroll(int x, int y) {
	distance = MAX(distance + y, 0);
}