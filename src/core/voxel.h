#pragma once

#include <cstdint>
#include <map>

#define VOXEL_BUFFER_LENGTH 128

class VoxelBuffer
{
public:

	VoxelBuffer();
	~VoxelBuffer();
	
	static uint32_t constructVoxel(uint8_t colour);

	bool setVoxel(uint32_t parent, uint8_t position, uint32_t voxel);
	bool delVoxel(uint32_t parent, uint8_t position) { return setVoxel(parent, position, 0x00000000); };
	bool movVoxel(uint32_t parentF, uint8_t positionF, uint32_t parentT, uint8_t positionT);

	uint32_t getVoxel(uint32_t index);

	void logVoxel(uint32_t index);

private:

	uint32_t* buffer;
	uint32_t size;
	std::map<uint32_t, uint32_t> spots;

	uint32_t allocBufferSpace(uint32_t length);
	void freeBufferSpace(uint32_t index, uint32_t length);

	void clearVoxel(uint32_t index, bool clearParent, bool clearChildren);
};