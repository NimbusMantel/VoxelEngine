#pragma once

#include <cstdint>

typedef uint32_t uint24_t;

// TO DO: include modifiable mask for multi update operations

namespace voxels {
	static const size_t size = sizeof(uint8_t) << 21;
	static uint8_t* const content = new uint8_t[size];

	static size_t SubgroupSize;

	void reset();

	struct __Interface {
		protected:
			static void submit(const uint8_t opcode, const size_t size, const uint8_t amount, uint8_t* data);

			__Interface() = default;
	};
}

template<uint8_t opcode, size_t size, uint8_t amount> struct VoxelUpdateOperation : private voxels::__Interface {
	public:
		friend void __submit(VoxelUpdateOperation<opcode, size, amount>& operation) {
			voxels::__Interface::submit(opcode, size, amount, operation.data);
		}

	protected:
		uint8_t * const data = new uint8_t[size];

		VoxelUpdateOperation() = default;
};

namespace voxels {
	template<uint8_t opcode, size_t size, uint8_t amount> void submit(VoxelUpdateOperation<opcode, size, amount>& operation) {
		__submit(operation);
	}
}

struct STR_CLD_M : VoxelUpdateOperation<0b0001, 3 + 8 * 1, 8> {
	STR_CLD_M(uint24_t address, uint8_t* masks);
};

struct VOX_CPY_M : VoxelUpdateOperation<0b0010, 3 + 8 * 3, 8> {
	VOX_CPY_M(uint24_t address, uint24_t* destinations);
};

struct VOX_CPY_S : VoxelUpdateOperation<0b0011, 3 + 3, 1> {
	VOX_CPY_S(uint24_t address, uint24_t destination);
};

struct STR_LOA_M : VoxelUpdateOperation<0b0100, 3 + 8 * (3 + 1), 8> {
	STR_LOA_M(uint24_t address, uint32_t* structures);
};

struct STR_CLD_S : VoxelUpdateOperation<0b0101, 3 + 1, 1> {
	STR_CLD_S(uint24_t address, uint8_t mask);
};

struct STR_PNT_S : VoxelUpdateOperation<0b0110, 3 + 3, 1> {
	STR_PNT_S(uint24_t address, uint24_t pointer);
};

struct STR_LOA_S : VoxelUpdateOperation<0b0111, 3 + (3 + 1), 1> {
	STR_LOA_S(uint24_t address, uint32_t structure);
};

struct MAT_LOA_M : VoxelUpdateOperation<0b1000, 3 + 8 * (8 + 4 + 4), 8> {
	MAT_LOA_M(uint24_t address, uint32_t* materials);
};

struct MAT_xxL_S : VoxelUpdateOperation<0b1001, 3 + 4, 1> {
	MAT_xxL_S(uint24_t address, uint32_t light);
};

struct MAT_xSx_S : VoxelUpdateOperation<0b1010, 3 + 4, 1> {
	MAT_xSx_S(uint24_t address, uint32_t surface);
};

struct MAT_xSL_S : VoxelUpdateOperation<0b1011, 3 + (4 + 4), 1> {
	MAT_xSL_S(uint24_t address, uint32_t surface, uint32_t light);
};

struct MAT_Cxx_S : VoxelUpdateOperation<0b1100, 3 + 8, 1> {
	MAT_Cxx_S(uint24_t address, uint32_t* colour);
};

struct MAT_CxL_S : VoxelUpdateOperation<0b1101, 3 + (8 + 4), 1> {
	MAT_CxL_S(uint24_t address, uint32_t* colour, uint32_t light);
};

struct MAT_CSx_S : VoxelUpdateOperation<0b1110, 3 + (8 + 4), 1> {
	MAT_CSx_S(uint24_t address, uint32_t* colour, uint32_t surface);
};

struct MAT_LOA_S : VoxelUpdateOperation<0b1111, 3 + (8 + 4 + 4), 1> {
	MAT_LOA_S(uint24_t address, uint32_t* colour, uint32_t surface, uint32_t light);
};
