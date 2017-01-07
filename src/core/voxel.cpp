#include "voxel.h"

#include "config.h"
#include "platform_log.h"

#include <stdlib.h>
#include <bitset>

// Voxel: 15 Bit first child - 1 Bit far pointer - 8 Bit children mask - 8 Bit colour index
// Parent: 16 Bit 0x0001 - 15 Bit parent - 1 Bit far pointer
// Pointer: 32 Bit pointer

// close pointers are always relative, far pointers always absolute

// TO DO: - voxel sparse tree transversal

VoxelBuffer::VoxelBuffer() {
	buffer = (uint32_t*)malloc(sizeof(uint32_t) * VOXEL_BUFFER_LENGTH);
	size = VOXEL_BUFFER_LENGTH;
	
	buffer[0] = 0xFFFF0000; // empty root voxel
	buffer[1] = 0x0001FFFE; // parent header pointing to the root voxel

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
		if (it->second >= length) { // Find a spot with enough length
			break;
		}
	}

	if (it == spots.end()) {
		void* ptr = realloc(buffer, size + ((VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length)); // Request additional buffer

		if (ptr == nullptr) {
			throw "Voxel buffer size increase error";
		}

		buffer = (uint32_t*)ptr;

		freeBufferSpace(size, (VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length); // Update the spots

		size += (VOXEL_BUFFER_LENGTH > length) ? VOXEL_BUFFER_LENGTH : length;

		it = spots.end()--; // Set the found spot to the last one, which has the newly allocated space
	}

	spots[it->first + length] = it->second - length;

	spots.erase(it);

	return it->first;
}

void VoxelBuffer::freeBufferSpace(uint32_t index, uint32_t length) {
	for (uint32_t i = index; i < (index + length); ++i) {
		buffer[i] = 0x00000000;
	}
	
	spots[index] = length;

	std::map<uint32_t, uint32_t>::iterator it = spots.find(index);

	it++;

	if (it != spots.end()) {
		if ((index + length) == it->first) { // Check if the new spot is a pre-extension of an existing one
			spots[index] += it->second;

			spots.erase(it);
		}
	}

	it--;

	if (it != spots.begin()) {
		it--;

		if ((it->first + it->second) == index) { // Check if the new spot is a post-extension of an existing one
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
	uint8_t i;

	if (clearChildren && ((Voxel & 0x0000FF00) >> 8) != 0) { // Voxel has children that are to be cleared
		Pointer = index + (int16_t)((Voxel & 0x7FFE0000) >> 17 | (Voxel & 0x80000000) >> 16 | (Voxel & 0x80000000) >> 17); // First child pointer
		
		if ((Voxel & 0x00010000) == 1) {
			Voxel = buffer[Pointer];

			freeBufferSpace(Pointer, 1); // Free the pointer to the first child

			Pointer = Voxel;

			Voxel = buffer[index];
		}
		
		for (i = 0; i < 8; ++i) {
			Pointer++;

			clearVoxel(Pointer, clearParent, true); // Clear the voxel's children
		}

		if ((buffer[Pointer] & 0x00000001) == 1) {
			freeBufferSpace(index + (int16_t)((buffer[Pointer] & 0x00007FFE) >> 1 | (buffer[Pointer] & 0x00008000) | (buffer[Pointer] & 0x00008000) >> 1), 1); // Remove the parent header's pointer to the voxel
		}

		freeBufferSpace(Pointer - 8, 9); // Free the buffer of the children
	}

	if (index > 1) { // Voxel isn't the root
		buffer[index] = 0x00000000;

		i = 0;

		while ((buffer[index + i] >> 16) != 1) {
			i++; // Get the distance to the parent header
		}

		Pointer = index + i + (int16_t)((buffer[index + i] & 0x00007FFE) >> 1 | (buffer[index + i] & 0x00008000) | (buffer[index + i] & 0x00008000) >> 1); // Pointer to the voxel's parent
		
		if (buffer[index + i] & 0x00000001) {
			Pointer = buffer[Pointer];
		}
		
		if (clearParent && (((buffer[Pointer] & (~(0x01 << (8 + i - 1)))) & 0x0000FF00) >> 8) == 0) {
			clearVoxel(Pointer, true, clearChildren); // Clear the voxel's parent
		}
		else {
			buffer[Pointer] &= ~(0x01 << (8 + i - 1)); // Update the parent's children mask
		}
	}
	else {
		buffer[index] = ((Voxel & 0x000000FF) | 0xFFFF0000); // Clear the root's children mask and reset it's first child pointer
	}
}

bool VoxelBuffer::setVoxel(uint32_t parent, uint8_t position, uint32_t voxel) {
	if ((voxel >> 16) == 1 || position > 7 || (voxel & 0x0000FF00) > 0) {
		return false;
	}

	uint32_t Parent = buffer[parent];

	if (!Parent || (Parent >> 17) == 0) {
		return false;
	}

	uint32_t FirstChild, Pointer;

	if (((Parent & 0x0000FF00) >> 8) == 0) { // Check if the parent still has no children
		if ((voxel >> 16) == 0) {
			return true;
		}

		Parent |= (0x01 << (8 + 7 - position)); // Update the parent's children mask

		FirstChild = allocBufferSpace(9);
		
		if (((FirstChild - parent) & 0xFFFFC000) && ((FirstChild - parent) & 0xFFFFC000) ^ 0xFFFFC000) { // Check if a pointer to the first child is necessary
			Pointer = allocBufferSpace(1);
			buffer[Pointer] = FirstChild;

			Parent = ((Parent & 0x0000FFFF) | 0x00010000 | ((Pointer - parent) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}
		else {
			Parent = ((Parent & 0x0000FFFF) | ((FirstChild - parent) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}

		buffer[parent] = Parent;
		buffer[FirstChild] = buffer[FirstChild + 1] = buffer[FirstChild + 2] = buffer[FirstChild + 3] = buffer[FirstChild + 4]
						   = buffer[FirstChild + 5] = buffer[FirstChild + 6] = buffer[FirstChild + 7] = 0x00000000;
		
		if (((parent - (FirstChild + 8)) & 0xFFFFC000) && ((parent - (FirstChild + 8)) & 0xFFFFC000) ^ 0xFFFFC000) { // Check if a pointer back to the parent is necessary
			Pointer = allocBufferSpace(1);
			buffer[Pointer] = parent;

			buffer[(FirstChild + 8)] = (0x00010001 | ((parent - Pointer) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
		else {
			buffer[(FirstChild + 8)] = (0x00010000 | ((parent - (FirstChild + 8)) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
	}
	else {
		FirstChild = parent + (int16_t)((Parent & 0x7FFE0000) >> 17 | (Parent & 0x80000000) >> 16 | (Parent & 0x80000000) >> 17);
		
		if (Parent & 0x00010000) {
			FirstChild = buffer[FirstChild];
		}

		if ((((Parent & 0x0000FF00) >> 8) & (0x01 << (7 - position))) > 0) { // Check if the parent already has a child at the position
			if (buffer[FirstChild + position] == voxel) {
				return true; // Return true if the child hasn't changed
			}

			if ((voxel >> 17) > 0) {
				return false; // Return false if a child would have to be replaced
			}
			else {
				clearVoxel(FirstChild + position, true, true); // Clear the child

				return true;
			}
		}
		else if ((voxel >> 17) == 0) {
			return true;
		}

		Parent |= (0x01 << (8 + 7 - position)); // Update the parent's children mask

		buffer[parent] = Parent;
	}

	buffer[FirstChild + position] = voxel; // Set the voxel in the buffer
	
	return true;
}

bool VoxelBuffer::movVoxel(uint32_t parentF, uint8_t positionF, uint32_t parentT, uint8_t positionT) {
	if (positionF > 7 || positionT > 7) {
		return false;
	}

	uint32_t ParentF = buffer[parentF];
	uint32_t ParentT = buffer[parentT];

	if (!ParentF || !ParentT || (ParentF >> 17) == 0 || (ParentT >> 17) == 0) {
		return false;
	}

	if ((((ParentF & 0x0000FF00) >> 8) & (0x01 << (7 - positionF))) == 0) {
		return false; // Return false if parentF doesn't have a child at the position
	}
	
	uint8_t layerF = 1;
	uint8_t layerT = 1;

	uint32_t pointer = parentF;

	uint8_t i;

	while (pointer != 0) {
		i = 0;
		
		while ((buffer[pointer + i] >> 16) != 1) {
			i++;
		}

		pointer = pointer + i + (int16_t)((buffer[pointer + i] & 0x00007FFE) >> 1 | (buffer[pointer + i] & 0x00008000) | (buffer[pointer + i] & 0x00008000) >> 1); // Traverse the tree from parentF to the root

		if (buffer[pointer + i] & 0x00000001) {
			pointer = buffer[pointer];
		}

		layerF++;
	}

	pointer = parentT;

	while (pointer != 0) {
		i = 0;

		while ((buffer[pointer + i] >> 16) != 1) {
			i++;
		}

		pointer = pointer + i + (int16_t)((buffer[pointer + i] & 0x00007FFE) >> 1 | (buffer[pointer + i] & 0x00008000) | (buffer[pointer + i] & 0x00008000) >> 1); // Traverse the tree from parentT to the root

		layerT++;
	}

	if (layerT != layerF) {
		return false; // Return false if the voxel would have to change its layer
	}

	uint32_t voxel = parentF + (int16_t)((ParentF & 0x7FFE0000) >> 17 | (ParentF & 0x80000000) >> 16 | (ParentF & 0x80000000) >> 17);

	if (ParentF & 0x00010000) {
		voxel = buffer[voxel];
	}

	if (!setVoxel(parentT, positionT, buffer[voxel] & 0xFFFF00FF)) {
		return false;
	}

	ParentT = buffer[parentT];

	if (((buffer[voxel] & 0x0000FF00) >> 8) > 0) {
		parentF = voxel + (int16_t)((buffer[voxel] & 0x7FFE0000) >> 17 | (buffer[voxel] & 0x80000000) >> 16 | (buffer[voxel] & 0x80000000) >> 17); // Pointer to the voxel's first child

		if (buffer[voxel] & 0x00010000) {
			parentF = buffer[parentF];
		}
		
		parentT = parentT + (int16_t)((ParentT & 0x7FFE0000) >> 17 | (ParentT & 0x80000000) >> 16 | (ParentT & 0x80000000) >> 17) + positionT; // Pointer to new position of the voxel
		
		if (ParentT & 0x00010000) {
			parentT = buffer[parentT];
		}
		
		if (buffer[parentT] & 0x00010000) {
			freeBufferSpace(parentT + (int16_t)((buffer[parentT] & 0x7FFE0000) >> 17 | (buffer[parentT] & 0x80000000) >> 16 | (buffer[parentT] & 0x80000000) >> 17), 1); // Free the previous pointer to the first child
		}
		
		if (((parentF - parentT) & 0xFFFFC000) && ((parentF - parentT) & 0xFFFFC000) ^ 0xFFFFC000) {
			pointer = allocBufferSpace(1);
			buffer[pointer] = parentF;

			buffer[parentT] = ((buffer[parentT] & 0x0000FFFF) | 0x00010000 | ((pointer - parentT) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}
		else {
			buffer[parentT] = ((buffer[parentT] & 0x0000FFFF) | ((parentF - parentT) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}

		if (buffer[parentF + 8] & 0x00000001) {
			freeBufferSpace(parentF + (int16_t)((buffer[parentF + 8] & 0x00007FFE) >> 1 | (buffer[parentF + 8] & 0x00008000) | (buffer[parentF + 8] & 0x00008000) >> 1), 1); // Free the previous pointer to the parent
		}
		
		if (((parentT - (parentF + 8)) & 0xFFFFC000) && ((parentT - (parentF + 8)) & 0xFFFFC000) ^ 0xFFFFC000) {
			pointer = allocBufferSpace(1);
			buffer[pointer] = parentT;

			buffer[(parentF + 8)] = (0x00010001 | ((parentT - pointer) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
		else {
			buffer[(parentF + 8)] = (0x00010000 | ((parentT - (parentF + 8)) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
	}

	clearVoxel(voxel, true, false); // Clear at the voxel's original position

	return true;
}

uint32_t VoxelBuffer::getVoxel(uint32_t index) {
	return ((index < size) ? buffer[index] : 0x00000000);
}

void VoxelBuffer::logVoxel(uint32_t index) {
	DEBUG_LOG_RAW("VoxelBuffer", "(%d) %s", index, std::bitset<32>(getVoxel(index)).to_string().c_str());
}

void VoxelBuffer::renderFrame(uint16_t width, uint16_t height, uint8_t* pixels) {
	for (uint16_t h = 0; h < height; ++h) {
		for (uint16_t w = 0; w < width; ++w) {
			pixels[w + h * width] = rand() * 255.0f / RAND_MAX;
		}
	}
}