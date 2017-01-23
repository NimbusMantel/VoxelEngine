#pragma once

#include "geo.h"

#include <cstdint>
#include <functional>
#include <map>

#define VOXEL_BUFFER_LENGTH 2000000

class VoxelBuffer
{
public:

	VoxelBuffer();
	~VoxelBuffer();
	
	static uint64_t constructVoxel(uint32_t colour);

	bool setVoxel(uint32_t parent, uint8_t position, uint64_t voxel);
	bool delVoxel(uint32_t parent, uint8_t position) { return setVoxel(parent, position, 0x0000000000000000); };
	bool movVoxel(uint32_t parentF, uint8_t positionF, uint32_t parentT, uint8_t positionT);
	bool addVoxel(int16_t posX, int16_t posY, int16_t posZ, uint16_t size, uint64_t voxel);

	uint64_t getVoxel(uint32_t index);

	void logVoxel(uint32_t index);

	std::function<void(mat4)> getRenderFunction(uint16_t width, uint16_t height, uint16_t fov, uint32_t* buffer, bool* mask);

private:

	uint32_t* buffer;
	uint32_t size;
	std::map<uint32_t, uint32_t> spots;

	uint32_t allocBufferSpace(uint32_t length);
	void freeBufferSpace(uint32_t index, uint32_t length);

	void clearVoxel(uint32_t index, bool clearParent, bool clearChildren);

	void frontToBack(uint32_t index, int16_t posX, int16_t posY, int16_t posZ, uint16_t size, const int16_t eyeX, const int16_t eyeY, const int16_t eyeZ, std::function<bool(int16_t, int16_t, int16_t, uint16_t, uint32_t, bool)>& render);
};