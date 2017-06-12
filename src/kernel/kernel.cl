// Typedefs

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;


// instruction language
/*KERNEL_INCLUDE_INS*/

// kernels

__kernel void cgProKernel(__global __read_write uint32_t* vxBuffer, __global __read_only char* cgBuffer, uint32_t syncInsAmount, uint32_t asyncInsAmount) {
	size_t tid = get_global_id(0);

	uint8_t insCode;
	uint32_t amount;
	uint32_t pointer;

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
		case INS_CTG_RLD_C: break;
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