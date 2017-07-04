// Typedefs

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long		uint64_t;


// macro definitions

#define INT_BUF(b, p) (((uint32_t)b[p] << 24) | ((uint32_t)b[p + 1] << 16) | ((uint32_t)b[p + 2] << 8) | b[p + 3])


// instruction language
/*KERNEL_INCLUDE_BEG*/

// enum values placeholders

enum PLACEHOLDER {
	INS_CTG_RLD_C,
	INS_CTG_ULD_C,
	INS_CTG_ADD_C,
	INS_CTG_REM_C,
	INS_CTG_MOV_C,
	INS_CTG_EXP_C,
	INS_CTG_COL_C,
	INS_CTG_LIT_C,

	INS_CTG_RLD_S,
	INS_CTG_ULD_S,
	INS_CTG_ADD_S,
	INS_CTG_REM_S,
	INS_CTG_MOV_S,
	INS_CTG_EXP_S,
	INS_CTG_COL_S,
	INS_CTG_LIT_S,

	INS_GTC_REQ_C,
	INS_GTC_SUG_C,

	INS_GTC_REQ_S,
	INS_GTC_SUG_S
};

// opencl debug placeholder

#define OpenCLDebug 1

/*KERNEL_INCLUDE_END*/

// function declarations

void cgLoad(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgUnload(__global uint32_t* vxBuffer, uint32_t parent);
void cgAdd(__global uint32_t* vxBuffer, uint32_t parent, uint32_t ca_0, uint32_t ca_1, uint32_t ca_2, uint32_t ca_3, uint32_t cb_0, uint32_t cb_1, uint32_t cb_2, uint32_t cb_3);
void cgRemove(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask);
void cgMove(__global uint32_t* vxBuffer, uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx);
void cgExpand(__global uint32_t* vxBuffer, uint32_t parent, uint32_t index);
void cgColour(__global uint32_t* vxBuffer, uint32_t index, uint32_t colour_0, uint32_t colour_1);
void cgLight(__global uint32_t* vxBuffer, uint32_t index, uint32_t light);


// kernels

__kernel void rayInitKernel(__global __write_only float* rvLookup, float pixWidth, float pixHeight, float halWidth, float halHeight, uint16_t width) {
	int2 pixel = { get_global_id(0), get_global_id(1) };
	
	float3 v = { pixel.x * pixWidth - halWidth, pixel.y * pixHeight - halHeight, -1.0f };

	vstore3(v / length(v), pixel.y * width + pixel.x, rvLookup);
}

__kernel void cgProKernel(__global __read_write uint32_t* vxBuffer, __global __read_only uint8_t* cgBuffer, uint32_t syncInsAmount, uint32_t asyncInsAmount) {
	size_t tid = get_global_id(0);

	uint8_t insCode;
	uint32_t amount;
	uint32_t pointer;
	uint32_t size;

	if (tid < syncInsAmount) {
		tid *= 7;

		insCode = cgBuffer[tid];
		amount = (cgBuffer[tid + 1] << 16) | (cgBuffer[tid + 2] << 8) | cgBuffer[tid + 3];
		pointer = (cgBuffer[tid + 4] << 16) | (cgBuffer[tid + 5] << 8) | cgBuffer[tid + 6];
		
		tid /= 7;
	}
	else {
		tid -= syncInsAmount;

		pointer = syncInsAmount * 7;
		amount = (cgBuffer[pointer + 1] << 16) | (cgBuffer[pointer + 2] << 8) | cgBuffer[pointer + 3];
		
		while (tid >= amount) {
			tid -= amount;

			pointer += 7;
			amount = (cgBuffer[pointer + 1] << 16) | (cgBuffer[pointer + 2] << 8) | cgBuffer[pointer + 3];
		}
		
		insCode = cgBuffer[pointer];
		pointer = (cgBuffer[pointer + 4] << 16) | (cgBuffer[pointer + 5] << 8) | cgBuffer[pointer + 6];
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

#if OpenCLDebug
	printf("cgProKernel(syn: %u, ayn: %u)\n   ins: %u\n   siz: %u\n   amt: %u\n   ptr: %u\n   sid: %u\n\n", syncInsAmount, asyncInsAmount, insCode, size, amount, pointer, tid);
#endif

	while (amount > 0) {
		switch (insCode) {
			case INS_CTG_RLD_C: cgLoad(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32), INT_BUF(cgBuffer, pointer + 36), INT_BUF(cgBuffer, pointer + 40), INT_BUF(cgBuffer, pointer + 44), INT_BUF(cgBuffer, pointer + 48), INT_BUF(cgBuffer, pointer + 52), INT_BUF(cgBuffer, pointer + 56), INT_BUF(cgBuffer, pointer + 60), INT_BUF(cgBuffer, pointer + 64), INT_BUF(cgBuffer, pointer + 68), INT_BUF(cgBuffer, pointer + 72), INT_BUF(cgBuffer, pointer + 76), INT_BUF(cgBuffer, pointer + 80), INT_BUF(cgBuffer, pointer + 84), INT_BUF(cgBuffer, pointer + 88), INT_BUF(cgBuffer, pointer + 92), INT_BUF(cgBuffer, pointer + 96), INT_BUF(cgBuffer, pointer + 100), INT_BUF(cgBuffer, pointer + 104), INT_BUF(cgBuffer, pointer + 108), INT_BUF(cgBuffer, pointer + 112), INT_BUF(cgBuffer, pointer + 116), INT_BUF(cgBuffer, pointer + 120), INT_BUF(cgBuffer, pointer + 124), INT_BUF(cgBuffer, pointer + 128), INT_BUF(cgBuffer, pointer + 132)); break;
			case INS_CTG_ULD_C: cgUnload(vxBuffer, INT_BUF(cgBuffer, pointer)); break;
			case INS_CTG_ADD_C: cgAdd(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32)); break;
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

__kernel void renderKernel(__global __read_only uint32_t* vxBuffer, __write_only image2d_t rbo, __global __read_only uint8_t* ldLookup, __global __read_only float* rvLookup) {
	__local uint8_t litDir[64];

	int ini = get_local_id(0) + get_local_size(0) * get_local_id(1);
	int stp = get_local_size(0) * get_local_size(1);

	for (uint8_t i = ini; i < 64; i += stp) {
		litDir[i] = ldLookup[i];
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	int2 pixel = { get_global_id(0), get_global_id(1) };

	float3 dir = vload3(pixel.y * get_image_width(rbo) + pixel.x, rvLookup);
	
	write_imagef(rbo, pixel, (float4)(dir.x / 2.0f + 0.5f, dir.y / 2.0f + 0.5f, dir.z / 2.0f + 0.5f, 1));
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

#if OpenCLDebug
	printf("cgLoad(parent: %u, index: %u)\n   ptr: %u\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, index, vxBuffer[(parent << 2) + 1], ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
#endif
}

void cgUnload(__global uint32_t* vxBuffer, uint32_t parent) {
	vxBuffer[(parent << 2) + 1] = 0x00;

#if OpenCLDebug
	printf("cgUnload(parent: %u)\n\n", parent);
#endif
}

void cgAdd(__global uint32_t* vxBuffer, uint32_t parent, uint32_t ca_0, uint32_t ca_1, uint32_t ca_2, uint32_t ca_3, uint32_t cb_0, uint32_t cb_1, uint32_t cb_2, uint32_t cb_3) {
	uint32_t idx = vxBuffer[(parent << 2) + 1] << 2;

	uint8_t adx = (ca_0 & 0x70000000) >> 26;
	uint8_t bdx = adx ^ ((((cb_0 & 0x70000000) >> 26) ^ adx) & -((bool)(cb_0 & 0x80000000)));
	
	vxBuffer[idx + bdx + 0] = cb_0; vxBuffer[idx + bdx + 1] = cb_1; vxBuffer[idx + bdx + 2] = cb_2; vxBuffer[idx + bdx + 3] = cb_3;
	vxBuffer[idx + adx + 0] = ca_0; vxBuffer[idx + adx + 1] = ca_1; vxBuffer[idx + adx + 2] = ca_2; vxBuffer[idx + adx + 3] = ca_3;

	vxBuffer[parent << 2] |= ((0x00800000 >> (adx >> 2)) | (0x00800000 >> (bdx >> 2)));

#if OpenCLDebug
	if (cb_0 & 0x80000000) {
		printf("cgAdd(parent: %u, aidx: %u, bidx: %u)\n   chm: 0x%02X\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, adx, bdx, (vxBuffer[parent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
	}
	else {
		printf("cgAdd(parent: %u, aidx: %u)\n   chm: 0x%02X\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, adx, (vxBuffer[parent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
	}
#endif
}

void cgRemove(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask) {
	uint32_t idx = vxBuffer[(parent << 2) + 1] << 2;

	uint8_t bdx = clz(mask) << 2;
	uint8_t cdx;

	cdx = bdx ^ ((0 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((4 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((8 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((12 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((16 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((20 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((24 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;
	cdx = bdx ^ ((28 ^ bdx) & -(mask & 0x80)); vxBuffer[idx + cdx + 1] &= 0x7F000000; vxBuffer[idx + cdx + 1] = 0x00; vxBuffer[idx + cdx + 2] = 0x00; vxBuffer[idx + cdx + 3] = 0x00;

	vxBuffer[parent << 2] &= ~((uint32_t)mask << 16);

#if OpenCLDebug
	printf("cgRemove(parent: %u, mask: 0x%02X)\n   chm: 0x%02X\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, mask, (vxBuffer[parent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
#endif
}

void cgMove(__global uint32_t* vxBuffer, uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx) {
	uint32_t fcidx = (vxBuffer[(fparent << 2) + 1] << 2) | (fidx << 2);
	uint32_t tcidx = (vxBuffer[(tparent << 2) + 1] << 2) | (tidx << 2);

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

	uint32_t ccidx = vxBuffer[tcidx + 1];

	vxBuffer[ccidx + 0] = (vxBuffer[ccidx + 0] & 0xF0FFFFFF) | ((tcidx & 0xF0000000) >> 4);
	vxBuffer[ccidx + 4] = (vxBuffer[ccidx + 4] & 0xF0FFFFFF) | (tcidx & 0x0F000000);
	vxBuffer[ccidx + 8] = (vxBuffer[fcidx + 8] & 0xF0FFFFFF) | ((tcidx & 0x00F00000) << 4);
	vxBuffer[ccidx + 12] = (vxBuffer[ccidx + 12] & 0xF0FFFFFF) | ((tcidx & 0x000F0000) << 8);
	vxBuffer[ccidx + 16] = (vxBuffer[ccidx + 16] & 0xF0FFFFFF) | ((tcidx & 0x0000F000) << 12);
	vxBuffer[ccidx + 20] = (vxBuffer[ccidx + 20] & 0xF0FFFFFF) | ((tcidx & 0x00000F00) << 16);
	vxBuffer[ccidx + 24] = (vxBuffer[ccidx + 24] & 0xF0FFFFFF) | ((tcidx & 0x000000F0) << 20);
	vxBuffer[ccidx + 28] = (vxBuffer[ccidx + 28] & 0xF0FFFFFF) | ((tcidx & 0x0000000F) << 24);

#if OpenCLDebug
	printf("cgMove(fparent: %u, fidx: %u, tparent: %u, tidx: %u)\n   fcm: 0x%02X\n   f_%u: 0x%016llX%016llX\n   tcm: 0x%02X\n   t_%u: 0x%016llX%016llX\n   tix: %u\npar: %u\n\n", fparent, fidx, tparent, tidx, (vxBuffer[fparent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[fcidx + 0] << 32) | vxBuffer[fcidx + 1], ((uint64_t)vxBuffer[fcidx + 2] << 32) | vxBuffer[fcidx + 3], (vxBuffer[tparent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[tcidx + 0] << 32) | vxBuffer[tcidx + 1], ((uint64_t)vxBuffer[tcidx + 2] << 32) | vxBuffer[tcidx + 3], vxBuffer[(tparent << 2) + 1] | tidx, ((vxBuffer[ccidx + 0] & 0x0F000000) << 4) | (vxBuffer[ccidx + 4] & 0x0F000000) | ((vxBuffer[ccidx + 8] & 0x0F000000) >> 4) | ((vxBuffer[ccidx + 12] & 0x0F000000) >> 8) | ((vxBuffer[ccidx + 16] & 0x0F000000) >> 12) | ((vxBuffer[ccidx + 20] & 0x0F000000) >> 16) | ((vxBuffer[ccidx + 24] & 0x0F000000) >> 20) | ((vxBuffer[ccidx + 28] & 0x0F000000) >> 24));
#endif
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

#if OpenCLDebug
	printf("cgExpand(parent: %u, index: %u)\n   ptr: %u\n\n", parent, index, vxBuffer[(parent << 2) + 1]);
#endif
}

void cgColour(__global uint32_t* vxBuffer, uint32_t index, uint32_t colour_0, uint32_t colour_1) {
	uint32_t idx = index << 2;
	
	vxBuffer[idx + 2] = colour_0;
	vxBuffer[idx + 3] = (vxBuffer[idx + 3] & 0x00FFFFFF) | colour_1;

#if OpenCLDebug
	printf("cgColour(index: %u)\n   col: 0x%08X%02X\n\n", index, vxBuffer[idx + 2], vxBuffer[idx + 3] >> 24);
#endif
}

void cgLight(__global uint32_t* vxBuffer, uint32_t index, uint32_t light) {
	uint32_t idx = index << 2;

	vxBuffer[idx + 3] = (vxBuffer[idx + 3] & 0xFF000000) | light;

#if OpenCLDebug
	printf("cgLight(index: %u)\n   lit: 0x%06X\n\n", index, vxBuffer[idx + 3] & 0x00FFFFFF);
#endif
}