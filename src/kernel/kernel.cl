// Typedefs

typedef unsigned char		uint8_t;
typedef unsigned int		uint32_t;


// macro definitions

#define INT_BUF(b, p) (((uint32_t)b[p] << 24) | ((uint32_t)b[p + 1] << 16) | ((uint32_t)b[p + 2] << 8) | b[p + 3])


// instruction language
/*KERNEL_INCLUDE_INS*/

// function declarations

void cgLoad(uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgUnload(uint32_t parent);
void cgAdd(uint32_t parent, uint8_t mask, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgRemove(uint32_t parent, uint8_t mask);
void cgMove(uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx);
void cgEmitter(uint32_t index, uint32_t emitter);
void cgColour(uint32_t index, uint32_t colour);
void cgLights(uint32_t index, uint32_t lights_0, uint32_t lights_1);


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
		case INS_CTG_EMI_C: size = INS_CTG_EMI_S; break;
		case INS_CTG_COL_C: size = INS_CTG_COL_S; break;
		case INS_CTG_LIT_C: size = INS_CTG_LIT_S; break;
	}

	if (tid >= syncInsAmount) {
		pointer += (tid * size);
		amount = 1;
	}

	while (amount > 0) {
		switch (insCode) {
			case INS_CTG_RLD_C: cgLoad(INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32), INT_BUF(cgBuffer, pointer + 36), INT_BUF(cgBuffer, pointer + 40), INT_BUF(cgBuffer, pointer + 44), INT_BUF(cgBuffer, pointer + 48), INT_BUF(cgBuffer, pointer + 52), INT_BUF(cgBuffer, pointer + 56), INT_BUF(cgBuffer, pointer + 60), INT_BUF(cgBuffer, pointer + 64), INT_BUF(cgBuffer, pointer + 68), INT_BUF(cgBuffer, pointer + 72), INT_BUF(cgBuffer, pointer + 76), INT_BUF(cgBuffer, pointer + 80), INT_BUF(cgBuffer, pointer + 84), INT_BUF(cgBuffer, pointer + 88), INT_BUF(cgBuffer, pointer + 92), INT_BUF(cgBuffer, pointer + 96), INT_BUF(cgBuffer, pointer + 100), INT_BUF(cgBuffer, pointer + 104), INT_BUF(cgBuffer, pointer + 108), INT_BUF(cgBuffer, pointer + 112), INT_BUF(cgBuffer, pointer + 116), INT_BUF(cgBuffer, pointer + 120), INT_BUF(cgBuffer, pointer + 124), INT_BUF(cgBuffer, pointer + 128), INT_BUF(cgBuffer, pointer + 132)); break;
			case INS_CTG_ULD_C: cgUnload(INT_BUF(cgBuffer, pointer)); break;
			case INS_CTG_ADD_C: cgAdd(INT_BUF(cgBuffer, pointer), cgBuffer[pointer + 4], INT_BUF(cgBuffer, pointer + 5), INT_BUF(cgBuffer, pointer + 9), INT_BUF(cgBuffer, pointer + 13), INT_BUF(cgBuffer, pointer + 17), INT_BUF(cgBuffer, pointer + 21), INT_BUF(cgBuffer, pointer + 25), INT_BUF(cgBuffer, pointer + 29), INT_BUF(cgBuffer, pointer + 33), INT_BUF(cgBuffer, pointer + 37), INT_BUF(cgBuffer, pointer + 41), INT_BUF(cgBuffer, pointer + 45), INT_BUF(cgBuffer, pointer + 49), INT_BUF(cgBuffer, pointer + 53), INT_BUF(cgBuffer, pointer + 57), INT_BUF(cgBuffer, pointer + 61), INT_BUF(cgBuffer, pointer + 65), INT_BUF(cgBuffer, pointer + 69), INT_BUF(cgBuffer, pointer + 73), INT_BUF(cgBuffer, pointer + 77), INT_BUF(cgBuffer, pointer + 81), INT_BUF(cgBuffer, pointer + 85), INT_BUF(cgBuffer, pointer + 89), INT_BUF(cgBuffer, pointer + 93), INT_BUF(cgBuffer, pointer + 97), INT_BUF(cgBuffer, pointer + 101), INT_BUF(cgBuffer, pointer + 105), INT_BUF(cgBuffer, pointer + 109), INT_BUF(cgBuffer, pointer + 113), INT_BUF(cgBuffer, pointer + 117), INT_BUF(cgBuffer, pointer + 121), INT_BUF(cgBuffer, pointer + 125), INT_BUF(cgBuffer, pointer + 129)); break;
			case INS_CTG_REM_C: cgRemove(INT_BUF(cgBuffer, pointer), cgBuffer[pointer + 4]); break; break;
			case INS_CTG_MOV_C: cgMove(INT_BUF(cgBuffer, pointer), ((cgBuffer[pointer + 8] & 0xE0) >> 5), INT_BUF(cgBuffer, pointer + 4), ((cgBuffer[pointer + 8] & 0x1C) >> 2)); break;
			case INS_CTG_EMI_C: cgEmitter(INT_BUF(cgBuffer, pointer), ((cgBuffer[pointer + 4] & 0x80) >> 7)); break;
			case INS_CTG_COL_C: cgColour(INT_BUF(cgBuffer, pointer), (((uint32_t)cgBuffer[pointer + 4] << 8) | cgBuffer[pointer + 5])); break;
			case INS_CTG_LIT_C: cgLights(INT_BUF(cgBuffer, pointer), ((uint32_t)INT_BUF(cgBuffer, pointer + 4) << 8), (((uint32_t)cgBuffer[pointer + 4] << 8) | cgBuffer[pointer + 5])); break;
		}

		amount -= 1;
		pointer += size;
	}
}

__kernel void render(__global __read_write uint32_t* vxBuffer, __write_only image2d_t rbo) {
	int width = get_image_width(rbo);
	int height = get_image_height(rbo);

	for (int h = 0; h < height; ++h) {
		for (int w = 0; w < width; ++w) {
			write_imagef(rbo, (int2)(w, h), (float4)(1, 1, 0, 1));
		}
	}
}


// functions

void cgLoad(uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3) {
	return;
}

void cgUnload(uint32_t parent) {
	return;
}

void cgAdd(uint32_t parent, uint8_t mask, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3) {
	return;
}

void cgRemove(uint32_t parent, uint8_t mask) {
	return;
}

void cgMove(uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx) {
	return;
}

void cgEmitter(uint32_t index, uint32_t emitter) {
	return;
}

void cgColour(uint32_t index, uint32_t colour) {
	return;
}

void cgLights(uint32_t index, uint32_t lights_0, uint32_t lights_1) {
	return;
}