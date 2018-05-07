#include "operations.hpp"

#include <cstdlib>

#define serialiseUint24(memory, offset, data, mask) memory[offset] = data >> 16; memory[offset + 1] = data >> 8; memory[offset + 2] = data & mask;
#define serialiseUint32(memory, offset, data) memory[offset] = data >> 24;  memory[offset + 1] = data >> 16; memory[offset + 2] = data >> 8; memory[offset + 3] = data;

BUF_END_S::BUF_END_S() {

}

STR_CLD_M::STR_CLD_M(uint24_t address, uint8_t* masks) {
	serialiseUint24(data, 0, address, 0xF8);

	memcpy(data + 3, masks, 8);
}

VOX_CPY_M::VOX_CPY_M(uint24_t address, uint24_t* destinations) {
	serialiseUint24(data, 0, address, 0xF8);

	for (int i = 0; i < 8; i++) {
		serialiseUint24(data, 3 + i * 3, destinations[i], 0xFF);
	}
}

VOX_CPY_S::VOX_CPY_S(uint24_t address, uint24_t destination) {
	serialiseUint24(data, 0, address, 0xFF);

	serialiseUint24(data, 3, destination, 0xFF);
}

STR_LOA_M::STR_LOA_M(uint24_t address, uint32_t* structures) {
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

MAT_LOA_M::MAT_LOA_M(uint24_t address, uint32_t* materials) {
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