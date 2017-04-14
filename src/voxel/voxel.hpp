#pragma once

#include <cstdint>
#include <vector>

/*
#define VOXEL_IS_ACTIVE(v) (       v      )	>> 31
#define VOXEL_SUB_INDEX(v) (v & 0x70000000) >> 28
#define VOXEL_PARENT_PT(v) (v & 0x0F000000) >> 24
#define VOXEL_CHILD_PNT(v) (v & 0x00FF0000) >> 16

#define VOXEL_PARENT_AD(v, i, c) ((VOXEL_PARENT_PT(v[i - (c << 2)]) << 28) | (VOXEL_PARENT_PT(v[i - (c << 2) + 4]) << 24) | (VOXEL_PARENT_PT(v[i - (c << 2) + 8]) << 20) | (VOXEL_PARENT_PT(v[i - (c << 2) + 12]) << 16) | (VOXEL_PARENT_PT(v[i - (c << 2) + 16]) << 12) | (VOXEL_PARENT_PT(v[i - (c << 2) + 20]) << 8) | (VOXEL_PARENT_PT(v[i - (c << 2) + 24]) << 4) | VOXEL_PARENT_PT(v[i - (c << 2) + 28]))
*/

#define BUFFER_DEPTH 23

#define BUFFER_SANITY 1

namespace manBuf {
	bool get(uint32_t pos);

	void set(uint32_t pos, bool toggled);

	void alo(uint32_t size, std::vector<std::pair<uint32_t, uint32_t>>& vec);
}