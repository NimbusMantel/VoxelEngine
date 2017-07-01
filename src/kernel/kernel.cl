// Typedefs

typedef unsigned char		uint8_t;
typedef unsigned int		uint32_t;


// macro definitions

#define INT_BUF(b, p) (((uint32_t)b[p] << 24) | ((uint32_t)b[p + 1] << 16) | ((uint32_t)b[p + 2] << 8) | b[p + 3])


// instruction language
/*KERNEL_INCLUDE_INS*/

// function declarations

void cgLoad(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgUnload(__global uint32_t* vxBuffer, uint32_t parent);
void cgAdd(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgRemove(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask);
void cgMove(__global uint32_t* vxBuffer, uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx);
void cgExpand(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index);
void cgColour(__global uint32_t* vxBuffer, uint32_t index, uint32_t colour_0, uint32_t colour_1);
void cgLight(__global uint32_t* vxBuffer, uint32_t index, uint32_t light);


// kernels

__kernel void cgProKernel(__global __read_write uint32_t* vxBuffer, __global __read_only char* cgBuffer, uint32_t syncInsAmount, uint32_t asyncInsAmount) {
	size_t tid = get_global_id(0);

	uint8_t insCode;
	uint32_t amount;
	uint32_t pointer;
	uint32_t size;

	if (tid < syncInsAmount) {
		insCode = cgBuffer[tid << 3];
		amount = (cgBuffer[(tid << 3) | 0x01] << 16) | (cgBuffer[(tid << 3) | 0x02] << 8) | cgBuffer[(tid << 3) | 0x03];
		pointer = (cgBuffer[(tid << 3) | 0x04] << 24) | (cgBuffer[(tid << 3) | 0x05] << 16) | (cgBuffer[(tid << 3) | 0x06] << 8) | cgBuffer[(tid << 3) | 0x07];
	}
	else {
		tid -= syncInsAmount;

		pointer = syncInsAmount << 3;
		amount = (cgBuffer[pointer | 0x01] << 16) | (cgBuffer[pointer | 0x02] << 8) | cgBuffer[pointer | 0x03];

		while (tid >= amount) {
			tid -= amount;

			pointer += 8;
			amount = (cgBuffer[pointer | 0x01] << 16) | (cgBuffer[pointer | 0x02] << 8) | cgBuffer[pointer | 0x03];
		}

		insCode = cgBuffer[pointer];
		pointer = (cgBuffer[pointer | 0x04] << 24) | (cgBuffer[pointer | 0x05] << 16) | (cgBuffer[pointer | 0x06] << 8) | cgBuffer[pointer | 0x07];
	}

	switch (insCode) {
		case INS_CTG_RLD_C: size = INS_CTG_RLD_S; break;
		case INS_CTG_ULD_C: size = INS_CTG_ULD_S; break;
		case INS_CTG_ADD_C: size = INS_CTG_ADD_S; break;
		case INS_CTG_REM_C: size = INS_CTG_REM_S; break;
		case INS_CTG_MOV_C: size = INS_CTG_MOV_S; break;
		case INS_CTG_EXP_C: size = INS_CTG_EXP_S; break;
		case INS_CTG_COL_C: size = INS_CTG_COL_S; break;
		case INS_CTG_LIT_C: size = INS_CTG_LIT_S; break;
	}

	if (tid >= syncInsAmount) {
		pointer += (tid * size);
		amount = 1;
	}

	while (amount > 0) {
		switch (insCode) {
			case INS_CTG_RLD_C: cgLoad(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32), INT_BUF(cgBuffer, pointer + 36), INT_BUF(cgBuffer, pointer + 40), INT_BUF(cgBuffer, pointer + 44), INT_BUF(cgBuffer, pointer + 48), INT_BUF(cgBuffer, pointer + 52), INT_BUF(cgBuffer, pointer + 56), INT_BUF(cgBuffer, pointer + 60), INT_BUF(cgBuffer, pointer + 64), INT_BUF(cgBuffer, pointer + 68), INT_BUF(cgBuffer, pointer + 72), INT_BUF(cgBuffer, pointer + 76), INT_BUF(cgBuffer, pointer + 80), INT_BUF(cgBuffer, pointer + 84), INT_BUF(cgBuffer, pointer + 88), INT_BUF(cgBuffer, pointer + 92), INT_BUF(cgBuffer, pointer + 96), INT_BUF(cgBuffer, pointer + 100), INT_BUF(cgBuffer, pointer + 104), INT_BUF(cgBuffer, pointer + 108), INT_BUF(cgBuffer, pointer + 112), INT_BUF(cgBuffer, pointer + 116), INT_BUF(cgBuffer, pointer + 120), INT_BUF(cgBuffer, pointer + 124), INT_BUF(cgBuffer, pointer + 128), INT_BUF(cgBuffer, pointer + 132)); break;
			case INS_CTG_ULD_C: cgUnload(vxBuffer, INT_BUF(cgBuffer, pointer)); break;
			case INS_CTG_ADD_C: cgAdd(vxBuffer, INT_BUF(cgBuffer, pointer), cgBuffer[pointer + 4], INT_BUF(cgBuffer, pointer + 5), INT_BUF(cgBuffer, pointer + 9), INT_BUF(cgBuffer, pointer + 13), INT_BUF(cgBuffer, pointer + 17), INT_BUF(cgBuffer, pointer + 21), INT_BUF(cgBuffer, pointer + 25), INT_BUF(cgBuffer, pointer + 29), INT_BUF(cgBuffer, pointer + 33), INT_BUF(cgBuffer, pointer + 37), INT_BUF(cgBuffer, pointer + 41), INT_BUF(cgBuffer, pointer + 45), INT_BUF(cgBuffer, pointer + 49), INT_BUF(cgBuffer, pointer + 53), INT_BUF(cgBuffer, pointer + 57), INT_BUF(cgBuffer, pointer + 61), INT_BUF(cgBuffer, pointer + 65), INT_BUF(cgBuffer, pointer + 69), INT_BUF(cgBuffer, pointer + 73), INT_BUF(cgBuffer, pointer + 77), INT_BUF(cgBuffer, pointer + 81), INT_BUF(cgBuffer, pointer + 85), INT_BUF(cgBuffer, pointer + 89), INT_BUF(cgBuffer, pointer + 93), INT_BUF(cgBuffer, pointer + 97), INT_BUF(cgBuffer, pointer + 101), INT_BUF(cgBuffer, pointer + 105), INT_BUF(cgBuffer, pointer + 109), INT_BUF(cgBuffer, pointer + 113), INT_BUF(cgBuffer, pointer + 117), INT_BUF(cgBuffer, pointer + 121), INT_BUF(cgBuffer, pointer + 125), INT_BUF(cgBuffer, pointer + 129)); break;
			case INS_CTG_REM_C: cgRemove(vxBuffer, INT_BUF(cgBuffer, pointer), cgBuffer[pointer + 4]); break; break;
			case INS_CTG_MOV_C: cgMove(vxBuffer, INT_BUF(cgBuffer, pointer), ((cgBuffer[pointer + 8] & 0xE0) >> 5), INT_BUF(cgBuffer, pointer + 4), ((cgBuffer[pointer + 8] & 0x1C) >> 2)); break;
			case INS_CTG_EXP_C: cgExpand(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4)); break;
			case INS_CTG_COL_C: cgColour(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), ((uint32_t)cgBuffer[pointer + 8] << 24)); break;
			case INS_CTG_LIT_C: cgLight(vxBuffer, INT_BUF(cgBuffer, pointer), (((uint32_t)cgBuffer[pointer + 4] << 16) | ((uint32_t)cgBuffer[pointer + 5] << 8) | cgBuffer[pointer + 6])); break;
		}

		amount -= 1;
		pointer += size;
	}
}

__kernel void render(__global __read_write uint32_t* vxBuffer, __write_only image2d_t rbo, __global __read_only uint8_t* ldLookup) {
	__local uint8_t litDir[64];

	int ini = get_local_id(0) + get_local_size(0) * get_local_id(1);
	int stp = get_local_size(0) * get_local_size(1);

	for (uint8_t i = ini; i < 64; i += stp) {
		litDir[i] = ldLookup[i];
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int width = get_image_width(rbo);
	int height = get_image_height(rbo);

	for (int h = 0; h < height; ++h) {
		for (int w = 0; w < width; ++w) {
			write_imagef(rbo, (int2)(w, h), (float4)(1, 1, 0, 1));
		}
	}
}


// functions

void cgLoad(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3) {
	uint32_t idx = index << 2;

	vxBuffer[idx + 0] = c0_0; vxBuffer[idx + 1] = c0_1; vxBuffer[idx + 2] = c0_2; vxBuffer[idx + 3] = c0_3;
	vxBuffer[idx + 4] = c1_0; vxBuffer[idx + 5] = c1_1; vxBuffer[idx + 6] = c1_2; vxBuffer[idx + 7] = c1_3;
	vxBuffer[idx + 8] = c2_0; vxBuffer[idx + 9] = c2_1; vxBuffer[idx + 10] = c2_2; vxBuffer[idx + 11] = c2_3;
	vxBuffer[idx + 12] = c3_0; vxBuffer[idx + 13] = c3_1; vxBuffer[idx + 14] = c3_2; vxBuffer[idx + 15] = c3_3;
	vxBuffer[idx + 16] = c4_0; vxBuffer[idx + 17] = c4_1; vxBuffer[idx + 18] = c4_2; vxBuffer[idx + 19] = c4_3;
	vxBuffer[idx + 20] = c5_0; vxBuffer[idx + 21] = c5_1; vxBuffer[idx + 22] = c5_2; vxBuffer[idx + 23] = c5_3;
	vxBuffer[idx + 24] = c6_0; vxBuffer[idx + 25] = c6_1; vxBuffer[idx + 26] = c6_2; vxBuffer[idx + 27] = c6_3;
	vxBuffer[idx + 28] = c7_0; vxBuffer[idx + 29] = c7_1; vxBuffer[idx + 30] = c7_2; vxBuffer[idx + 31] = c7_3;

	vxBuffer[(parent << 2) + 1] = index;
}

void cgUnload(__global uint32_t* vxBuffer, uint32_t parent) {
	vxBuffer[(parent << 2) + 1] = 0x00;
}

void cgAdd(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3) {
	uint32_t idx = vxBuffer[(parent << 2) + 1];
	
	if (mask & 0x80) { vxBuffer[idx + 0] = c0_0; vxBuffer[idx + 1] = c0_1; vxBuffer[idx + 2] = c0_2; vxBuffer[idx + 3] = c0_3; }
	if (mask & 0x40) { vxBuffer[idx + 4] = c1_0; vxBuffer[idx + 5] = c1_1; vxBuffer[idx + 6] = c1_2; vxBuffer[idx + 7] = c1_3; }
	if (mask & 0x20) { vxBuffer[idx + 8] = c2_0; vxBuffer[idx + 9] = c2_1; vxBuffer[idx + 10] = c2_2; vxBuffer[idx + 11] = c2_3; }
	if (mask & 0x10) { vxBuffer[idx + 12] = c3_0; vxBuffer[idx + 13] = c3_1; vxBuffer[idx + 14] = c3_2; vxBuffer[idx + 15] = c3_3; }
	if (mask & 0x08) { vxBuffer[idx + 16] = c4_0; vxBuffer[idx + 17] = c4_1; vxBuffer[idx + 18] = c4_2; vxBuffer[idx + 19] = c4_3; }
	if (mask & 0x04) { vxBuffer[idx + 20] = c5_0; vxBuffer[idx + 21] = c5_1; vxBuffer[idx + 22] = c5_2; vxBuffer[idx + 23] = c5_3; }
	if (mask & 0x02) { vxBuffer[idx + 24] = c6_0; vxBuffer[idx + 25] = c6_1; vxBuffer[idx + 26] = c6_2; vxBuffer[idx + 27] = c6_3; }
	if (mask & 0x01) { vxBuffer[idx + 28] = c7_0; vxBuffer[idx + 29] = c7_1; vxBuffer[idx + 30] = c7_2; vxBuffer[idx + 31] = c7_3; }

	vxBuffer[parent << 2] |= ((uint32_t)mask << 16);
}

void cgRemove(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask) {
	uint32_t idx = vxBuffer[(parent << 2) + 1];

	if (mask & 0x80) { vxBuffer[idx + 0] &= 0x7F000000; vxBuffer[idx + 1] = 0x00; vxBuffer[idx + 2] = 0x00; vxBuffer[idx + 3] = 0x00; }
	if (mask & 0x40) { vxBuffer[idx + 4] &= 0x7F000000; vxBuffer[idx + 5] = 0x00; vxBuffer[idx + 6] = 0x00; vxBuffer[idx + 7] = 0x00; }
	if (mask & 0x20) { vxBuffer[idx + 8] &= 0x7F000000; vxBuffer[idx + 9] = 0x00; vxBuffer[idx + 10] = 0x00; vxBuffer[idx + 11] = 0x00; }
	if (mask & 0x10) { vxBuffer[idx + 12] &= 0x7F000000; vxBuffer[idx + 13] = 0x00; vxBuffer[idx + 14] = 0x00; vxBuffer[idx + 15] = 0x00; }
	if (mask & 0x08) { vxBuffer[idx + 16] &= 0x7F000000; vxBuffer[idx + 17] = 0x00; vxBuffer[idx + 18] = 0x00; vxBuffer[idx + 19] = 0x00; }
	if (mask & 0x04) { vxBuffer[idx + 20] &= 0x7F000000; vxBuffer[idx + 21] = 0x00; vxBuffer[idx + 22] = 0x00; vxBuffer[idx + 23] = 0x00; }
	if (mask & 0x02) { vxBuffer[idx + 24] &= 0x7F000000; vxBuffer[idx + 25] = 0x00; vxBuffer[idx + 26] = 0x00; vxBuffer[idx + 27] = 0x00; }
	if (mask & 0x01) { vxBuffer[idx + 28] &= 0x7F000000; vxBuffer[idx + 29] = 0x00; vxBuffer[idx + 30] = 0x00; vxBuffer[idx + 31] = 0x00; }

	vxBuffer[parent << 2] &= ~((uint32_t)mask << 16);
}

void cgMove(__global uint32_t* vxBuffer, uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx) {
	uint32_t fcidx = vxBuffer[(fparent << 2) + 1] | (fidx << 2);
	uint32_t tcidx = vxBuffer[(tparent << 2) + 1] | (tidx << 2);

	vxBuffer[fparent << 2] &= ~(0x00800000 >> fidx);
	vxBuffer[tparent << 2] |= (0x00800000 >> tidx);

	vxBuffer[tcidx + 0] = (vxBuffer[fcidx + 0] & 0x80FFFFFF) | (tidx << 28) | (((tparent >> ((7 - tidx) << 2)) & 0x000000FF) << 24);
	vxBuffer[tcidx + 1] = vxBuffer[fcidx + 1];
	vxBuffer[tcidx + 2] = vxBuffer[fcidx + 2];
	vxBuffer[tcidx + 3] = vxBuffer[fcidx + 3];

	vxBuffer[fcidx + 0] &= 0x7F000000;
	vxBuffer[fcidx + 1] = 0x00;
	vxBuffer[fcidx + 2] = 0x00;
	vxBuffer[fcidx + 3] = 0x00;

	fcidx = vxBuffer[tcidx + 1];

	vxBuffer[fcidx + 0] = (vxBuffer[fcidx + 0] & 0xF0FFFFFF) | ((tcidx & 0xF0000000) >> 4);
	vxBuffer[fcidx + 4] = (vxBuffer[fcidx + 4] & 0xF0FFFFFF) | (tcidx & 0x0F000000);
	vxBuffer[fcidx + 8] = (vxBuffer[fcidx + 8] & 0xF0FFFFFF) | ((tcidx & 0x00F00000) << 4);
	vxBuffer[fcidx + 12] = (vxBuffer[fcidx + 12] & 0xF0FFFFFF) | ((tcidx & 0x000F0000) << 8);
	vxBuffer[fcidx + 16] = (vxBuffer[fcidx + 16] & 0xF0FFFFFF) | ((tcidx & 0x0000F000) << 12);
	vxBuffer[fcidx + 20] = (vxBuffer[fcidx + 20] & 0xF0FFFFFF) | ((tcidx & 0x00000F00) << 16);
	vxBuffer[fcidx + 24] = (vxBuffer[fcidx + 24] & 0xF0FFFFFF) | ((tcidx & 0x000000F0) << 20);
	vxBuffer[fcidx + 28] = (vxBuffer[fcidx + 28] & 0xF0FFFFFF) | ((tcidx & 0x0000000F) << 24);
}

void cgExpand(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index) {
	uint32_t idx = index << 2;
	
	vxBuffer[idx + 0] = (0x00000000 | ((parent & 0xF0000000) >> 4)); vxBuffer[idx + 1] = 0x00; vxBuffer[idx + 2] = 0x00; vxBuffer[idx + 3] = 0x00;
	vxBuffer[idx + 4] = (0x10000000 | (parent & 0x0F000000)); vxBuffer[idx + 5] = 0x00; vxBuffer[idx + 6] = 0x00; vxBuffer[idx + 7] = 0x00;
	vxBuffer[idx + 8] = (0x20000000 | ((parent & 0x00F00000) << 4)); vxBuffer[idx + 9] = 0x00; vxBuffer[idx + 10] = 0x00; vxBuffer[idx + 11] = 0x00;
	vxBuffer[idx + 12] = (0x30000000 | ((parent & 0x000F0000) << 8)); vxBuffer[idx + 13] = 0x00; vxBuffer[idx + 14] = 0x00; vxBuffer[idx + 15] = 0x00;
	vxBuffer[idx + 16] = (0x40000000 | ((parent & 0x0000F000) << 12)); vxBuffer[idx + 17] = 0x00; vxBuffer[idx + 18] = 0x00; vxBuffer[idx + 19] = 0x00;
	vxBuffer[idx + 20] = (0x50000000 | ((parent & 0x00000F00) << 16)); vxBuffer[idx + 21] = 0x00; vxBuffer[idx + 22] = 0x00; vxBuffer[idx + 23] = 0x00;
	vxBuffer[idx + 24] = (0x60000000 | ((parent & 0x000000F0) << 20)); vxBuffer[idx + 25] = 0x00; vxBuffer[idx + 26] = 0x00; vxBuffer[idx + 27] = 0x00;
	vxBuffer[idx + 28] = (0x70000000 | ((parent & 0x0000000F) << 24)); vxBuffer[idx + 29] = 0x00; vxBuffer[idx + 30] = 0x00; vxBuffer[idx + 31] = 0x00;

	vxBuffer[(parent << 2) + 1] = index;
}

void cgColour(__global uint32_t* vxBuffer, uint32_t index, uint32_t colour_0, uint32_t colour_1) {
	uint32_t idx = index << 2;
	
	vxBuffer[idx + 2] = colour_0;
	vxBuffer[idx + 3] = (vxBuffer[idx + 3] & 0x00FFFFFF) | colour_1;
}

void cgLight(__global uint32_t* vxBuffer, uint32_t index, uint32_t light) {
	uint32_t idx = index << 2;

	vxBuffer[idx + 3] = (vxBuffer[idx + 3] & 0xFF000000) | light;
}