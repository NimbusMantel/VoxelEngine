#include "operations.hpp"

#include <cstring>

#define serialiseUint24(memory, offset, data, mask) memory[offset] = data >> 16; memory[offset + 1] = data >> 8; memory[offset + 2] = data & mask;
#define serialiseUint32(memory, offset, data) memory[offset] = data >> 24;  memory[offset + 1] = data >> 16; memory[offset + 2] = data >> 8; memory[offset + 3] = data;

STR_CLD_M::STR_CLD_M(uint24_t address, uint8_t* masks, uint8_t usage) : VoxelUpdateOperation(usage) {
	serialiseUint24(data, 0, address, 0xF8);

	memcpy(data + 3, masks, 8);
}

VOX_CPY_M::VOX_CPY_M(uint24_t address, uint24_t* destinations, uint8_t usage) : VoxelUpdateOperation(usage) {
	serialiseUint24(data, 0, address, 0xF8);

	for (int i = 0; i < 8; i++) {
		serialiseUint24(data, 3 + i * 3, destinations[i], 0xFF);
	}
}

VOX_CPY_S::VOX_CPY_S(uint24_t address, uint24_t destination) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint24(data, 3, destination, 0xFF);
}

STR_LOA_M::STR_LOA_M(uint24_t address, uint32_t* structures, uint8_t usage) : VoxelUpdateOperation(usage) {
	serialiseUint24(data, 0, address, 0xF8);

	for (int i = 0; i < 8; i++) {
		serialiseUint32(data, 3 + i * 4, structures[i]);
	}
}

STR_CLD_S::STR_CLD_S(uint24_t address, uint8_t mask) {
	serialiseUint24(data, 0, address, 0xFF);

	data[4] = mask;
}

STR_PNT_S::STR_PNT_S(uint24_t address, uint24_t pointer) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint24(data, 3, pointer, 0xFF);
}

STR_LOA_S::STR_LOA_S(uint24_t address, uint32_t structure) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, structure);
}

MAT_LOA_M::MAT_LOA_M(uint24_t address, uint32_t* materials, uint8_t usage) : VoxelUpdateOperation(usage) {
	serialiseUint24(data, 0, address, 0xF8);

	for (int i = 0; i < 32; i++) {
		serialiseUint32(data, 3 + i * 4, materials[i]);
	}
}

MAT_xxL_S::MAT_xxL_S(uint24_t address, uint32_t light) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, light);
}

MAT_xSx_S::MAT_xSx_S(uint24_t address, uint32_t surface) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, surface);
}

MAT_xSL_S::MAT_xSL_S(uint24_t address, uint32_t surface, uint32_t light) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, surface);
	serialiseUint32(data, 7, light);
}

MAT_Cxx_S::MAT_Cxx_S(uint24_t address, uint32_t* colour) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, colour[0]);
	serialiseUint32(data, 7, colour[1]);
}

MAT_CxL_S::MAT_CxL_S(uint24_t address, uint32_t* colour, uint32_t light) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, colour[0]);
	serialiseUint32(data, 7, colour[1]);
	serialiseUint32(data, 11, light);
}

MAT_CSx_S::MAT_CSx_S(uint24_t address, uint32_t* colour, uint32_t surface) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, colour[0]);
	serialiseUint32(data, 7, colour[1]);
	serialiseUint32(data, 11, surface);
}

MAT_LOA_S::MAT_LOA_S(uint24_t address, uint32_t* colour, uint32_t surface, uint32_t light) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint32(data, 3, colour[0]);
	serialiseUint32(data, 7, colour[1]);
	serialiseUint32(data, 11, surface);
	serialiseUint32(data, 15, light);
}

struct OperationGroup {
	size_t begin;
	size_t index = 0;
};

static OperationGroup groups[15];

size_t subgroupSize;
size_t nextOffset = 0;

uint8_t* const content = new uint8_t[voxels::size];

// TO DO: Include fallback if command would not fit in the buffer anymore

namespace voxels {
	uint8_t* init(size_t sgs) {
		subgroupSize = sgs;

		reset();

		return content;
	}

	void reset() {
		nextOffset = 0;

		content[0] = 0x00;

		for (int i = 0; i < 15; i++) {
			groups[i].begin = groups[i].index = 0;
		}
	}

	void __Interface::submit(const uint8_t opcode, const size_t size, const uint8_t amount, uint8_t* data, uint8_t usage) {
		OperationGroup& group = groups[opcode - 1];

		if (group.index == 0) {
			group.begin = nextOffset;
			nextOffset += 1 + (subgroupSize >> 3) + size * (subgroupSize / amount);
			
			content[group.begin] = opcode;
			content[nextOffset] = 0x00;

			for (size_t off = 1; off < (subgroupSize >> 3); off += 1 + (size * 8 / amount)) {
				content[group.begin + off] = 0x00;
			}
		}
		
		content[group.begin + 1 + (group.index >> 3) + (size * 8 * (group.index >> 3))] |= (usage >> (group.index & 0x07));

		memcpy(content + group.begin + 1 + 1 + (group.index >> 3) + (size * group.index / amount), data, size);

		group.index += amount;

		if (group.index >= subgroupSize) {
			group.index = 0;
		}
	}
}
