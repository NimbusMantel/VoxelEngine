#include "voxel.h"

#include <stdlib.h>
#include <math.h>

// Voxel: 15 Bit first child - 1 Bit far pointer - 8 Bit children mask - 8 Bit colour index
// Parent: 16 Bit 0x0001 - 15 Bit parent - 1 Bit far pointer
// Pointer: 32 Bit pointer

// close pointers are always relative, far pointers always absolute

VoxelBuffer::VoxelBuffer() {
	buffer = (uint32_t*)malloc(sizeof(uint32_t) * VOXEL_BUFFER_LENGTH);
	size = VOXEL_BUFFER_LENGTH;
	
	buffer[0] = 0xFFFF0000;
	buffer[1] = 0x00010000;

	spots[2] = VOXEL_BUFFER_LENGTH - 2;
}

VoxelBuffer::~VoxelBuffer() {
	free(buffer);
}

uint32_t VoxelBuffer::constructVoxel(uint8_t colour) {
	return (0xFFFF0000 | colour);
}

uint32_t VoxelBuffer::allocBufferSpace(uint32_t length) {
	std::map<uint32_t, uint32_t>::iterator it;
	
	for (it = spots.begin(); it != spots.end(); ++it) {
		if (it->second >= length) {
			break;
		}
	}

	if (it == spots.end()) {
		void* ptr = realloc(buffer, size + ((VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length));

		if (ptr == nullptr) {
			throw "Voxel buffer size increase error";
		}

		buffer = (uint32_t*)ptr;

		freeBufferSpace(size, (VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length);

		size += (VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length;

		it = spots.end()--;
	}

	spots[it->first + length] = it->second - length;

	spots.erase(it);

	return it->first;
}

void VoxelBuffer::freeBufferSpace(uint32_t index, uint32_t length) {
	spots[index] = length;

	std::map<uint32_t, uint32_t>::iterator it = spots.find(index);

	if (it != spots.begin()) {
		it--;

		if ((it->first + it->second) == index) {
			it->second += length;

			spots.erase(++it);
		}
	}
}

void VoxelBuffer::clearVoxel(uint32_t index, bool clearParent, bool clearChildren) {
	uint32_t Voxel = buffer[index];

	if (!Voxel || (Voxel >> 17) == 0) {
		return;
	}

	uint32_t Pointer;

	if (clearChildren && ((Voxel & 0x0000FF00) >> 8) != 0) {
		Pointer = (((Voxel & 0x00010000) == 0) ? (index + (int32_t)(Voxel >> 17)) : (buffer[index + (int32_t)(Voxel >> 17)]));

		if ((Voxel & 0x00010000) == 1) {
			freeBufferSpace(index + (int32_t)(Voxel >> 17), 1);
		}

		for (; Pointer < (Pointer + 8); ++Pointer) {
			clearVoxel(Pointer, clearParent, true);
		}

		if ((buffer[Pointer] & 0x00000001) == 1) {
			freeBufferSpace((buffer[Pointer] & 0x0000FFFE) >> 1, 1);
		}

		freeBufferSpace(Pointer - 8, 9);
	}

	if (index > 1) {
		buffer[index] = 0x00000000;

		uint8_t i;

		while ((buffer[index + i] >> 16) != 1) {
			i++;
		}

		Pointer = (((buffer[index + i] & 0x00000001) == 0) ? (index + (int32_t)((buffer[index + i] & 0x0000FFFE) >> 1)) : buffer[index + (int32_t)((buffer[index + i] & 0x0000FFFE) >> 1)]);

		if (clearParent && (((buffer[Pointer] & (~((uint8_t)pow(2, i - 1) << 8))) & 0x0000FF00) >> 8) == 0) {
			clearVoxel(Pointer, true, clearChildren);
		}
		else {
			buffer[Pointer] &= ~((uint8_t)pow(2, i - 1) << 8);
		}
	}
	else {
		buffer[index] = ((Voxel & 0x000000FF) | 0xFFFF0000);
	}
	
	return;
}

bool VoxelBuffer::setVoxel(uint32_t parent, uint8_t position, uint32_t voxel) {
	return false; // TO DO
}

bool VoxelBuffer::movVoxel(uint32_t parentA, uint8_t positionA, uint32_t parentB, uint8_t positionB) {
	return false; // TO DO
}