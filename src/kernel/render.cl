// Typedefs

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long		uint64_t;

typedef int					int32_t;

// Macro definitions

#if CL_LITTLE_ENDIAN
#define LittleEndian(n) (n)
#else
#define LittleEndian(n) (rotate(n & 0x00FF00FF, 24U) | (rotate(n, 8U) & 0x00FF00FF))
#endif

#define uint24_b(b, p) (((uint32_t)b[p] << 16) | ((uint32_t)b[p + 1] << 8) | b[p + 2])
#define uint32_b(b, p) (((uint32_t)b[p] << 24) | ((uint32_t)b[p + 1] << 16) | ((uint32_t)b[p + 2] << 8) | b[p + 3])

// Instruction language

enum INS_CTG {
	INS_LST = 0x0,
	INS_UST = 0x1,
	INS_LMT = 0x2,
	INS_UMT = 0x3,
	INS_UCH = 0x4,
	INS_CPY = 0x5,
	INS_UPT = 0x6,
	INS_UME = 0x7,
	INS_LLT = 0x8
//  INS_ULT = 0x9 - 0xF
};

// Helper functions

inline int2 decodeAddress(const uint32_t address) {
	const uint8_t extInY = ((address ^ (address >> 10)) & 0x000010) >> 3;
	const uint8_t extInX = extInY ^ 0x02;

	return (int2)(((address >> 3) & 0x0007FE) | (extInY & (address >> 2)) | (extInX & (address >> 1)) | (address & 0x000001),
		         ((address >> 13) & 0x0007FE) | (extInX & (address >> 2)) | (extInY & (address >> 1)) | ((address & 0x000002) >> 1));
}

// Kernels

struct CmdUnit {
	uint8_t  opc;
	uint32_t amt;
};

const sampler_t voxSampler = (CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST);

kernel void voxUpdKernel(read_only image2d_t voxStrR, write_only image2d_t voxStrW,
	                     read_only image2d_t voxMatR, write_only image2d_t voxMatW,
	                     read_only image2d_t voxLitR, write_only image2d_t voxLitW,
	                     global read_only uint8_t* comBuf, volatile global read_write uint32_t* cmdCnt, local read_write struct CmdUnit* cmds) {
	local uint8_t  cmdIdx;
	local uint32_t idxBeg;
	local uint32_t adrBeg;
	local uint32_t wrkIdx;
	
	const uint32_t lid = get_local_id(0);

	const uint8_t osiz[16] = { 35, 7, 35, 7, 4, 6, 6, 6, 151, 7, 7, 11, 7, 11, 11, 15 };

	for (uint8_t i = lid; i < comBuf[0]; i += get_local_size(0)) {
		cmds[i].opc = comBuf[i * 3 + 1] >> 4;
		cmds[i].amt = ((uint32_t)(comBuf[i * 3 + 1] & 0x0F) << 16) | ((uint32_t)comBuf[i * 3 + 2] << 8) | comBuf[i * 3 + 3];
	}

	cmdIdx = idxBeg = 0;
	adrBeg = comBuf[0] * 3 + 1;

	barrier(CLK_LOCAL_MEM_FENCE);

	uint8_t  exeOpc;
	uint32_t exeAdr;

	int2 adrA = { 0, 0 };
	int2 adrB = { 0, 0 };
	uint4 data = { 0, 0, 0, 1 };

	while (true) {
		if (lid == 0) {
			while (true) {
				while ((idxBeg + cmds[cmdIdx].amt) < *cmdCnt) {
					idxBeg += cmds[cmdIdx].amt;
					adrBeg += cmds[cmdIdx].amt * osiz[cmds[cmdIdx].opc];
				}

				if (cmdIdx >= comBuf[0]) {
					break;
				}

				do {
					wrkIdx = *cmdCnt;
				} while (wrkIdx < (idxBeg + cmds[cmdIdx].amt) && atomic_cmpxchg(cmdCnt, wrkIdx, min((uint32_t)(wrkIdx + get_local_size(0)), (uint32_t)(idxBeg + cmds[cmdIdx].amt))) != wrkIdx);

				if (wrkIdx < (idxBeg + cmds[cmdIdx].amt)) {
					break;
				}
			}
		}

		barrier(CLK_LOCAL_MEM_FENCE);

		if (cmdIdx >= comBuf[0]) {
			break;
		}

		if (lid < (idxBeg + cmds[cmdIdx].amt - wrkIdx)) {
			exeOpc = cmds[cmdIdx].opc;
			exeAdr = adrBeg + (wrkIdx + lid - idxBeg) * osiz[exeOpc];

			if (exeOpc == INS_LST) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr) & 0xFFFFF8);
				data.x = uint32_b(comBuf, exeAdr + 3);
				data.y = data.z = 0;
				write_imageui(voxStrW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 7);
				write_imageui(voxStrW, adrA, data);

				adrA ^= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 11);
				write_imageui(voxStrW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 15);
				write_imageui(voxStrW, adrA, data);

				adrA ^= 0x01;
				adrA.x |= 0x20;
				data.x = uint32_b(comBuf, exeAdr + 19);
				write_imageui(voxStrW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 23);
				write_imageui(voxStrW, adrA, data);

				adrA ^= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 27);
				write_imageui(voxStrW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 31);
				write_imageui(voxStrW, adrA, data);
			}
			else if (exeOpc == INS_UST) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr));

				data.x = uint32_b(comBuf, exeAdr + 3);
				data.y = data.z = 0;

				write_imageui(voxStrW, adrA, data);
			}
			else if (exeOpc == INS_LMT) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr) & 0xFFFFF8);
				data.x = uint32_b(comBuf, exeAdr + 3);
				data.y = data.z = 0;
				write_imageui(voxMatW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 7);
				write_imageui(voxMatW, adrA, data);

				adrA ^= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 11);
				write_imageui(voxMatW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 15);
				write_imageui(voxMatW, adrA, data);

				adrA ^= 0x01;
				adrA.x |= 0x20;
				data.x = uint32_b(comBuf, exeAdr + 19);
				write_imageui(voxMatW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 23);
				write_imageui(voxMatW, adrA, data);

				adrA ^= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 27);
				write_imageui(voxMatW, adrA, data);

				adrA.x |= 0x01;
				data.x = uint32_b(comBuf, exeAdr + 31);
				write_imageui(voxMatW, adrA, data);
			}
			else if (exeOpc == INS_UMT) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr));

				data.x = uint32_b(comBuf, exeAdr + 3);
				data.y = data.z = 0;

				write_imageui(voxMatW, adrA, data);
			}
			else if (exeOpc == INS_UCH) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr));

				data = read_imageui(voxStrR, voxSampler, adrA);

				data.x &= 0xFFFFFF00;
				data.x |= comBuf[exeAdr + 3];

				write_imageui(voxStrW, adrA, data);
			}
			else if (exeOpc == INS_CPY) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr));
				adrB = decodeAddress(uint24_b(comBuf, exeAdr + 3));

				data = read_imageui(voxStrR, voxSampler, adrA);
				write_imageui(voxStrW, adrB, data);

				data = read_imageui(voxMatR, voxSampler, adrA);
				write_imageui(voxMatW, adrB, data);
			}
			else if (exeOpc == INS_UPT || exeOpc == INS_UME) {
				adrA = decodeAddress(uint24_b(comBuf, exeAdr));

				data = read_imageui(voxStrR, voxSampler, adrA);

				data.x &= 0x600000FF;
				data.x |= ((exeOpc == INS_UPT) << 31) | (uint24_b(comBuf, exeAdr + 3) << 8);

				write_imageui(voxStrW, adrA, data);
			}
			else if (exeOpc == INS_LLT) {
				// TO DO
			}
			else {
				adrA = (int2)(((uint32_t)(comBuf[exeAdr + 1] & 0x0F) << 8) | comBuf[exeAdr + 2], ((uint32_t)comBuf[exeAdr] << 4) | (comBuf[exeAdr + 1] >> 4));

				data.x = data.x ^ ((data.x ^ uint32_b(comBuf, exeAdr + 3)) & -(bool)(exeOpc & 0x4));
				data.y = data.y ^ ((data.y ^ uint32_b(comBuf, exeAdr + 3 + (exeOpc & 0x4))) & -(bool)(exeOpc & 0x2));
				data.z = data.z ^ ((data.z ^ uint32_b(comBuf, exeAdr + 3 + ((exeOpc + 2) & 0xC))) & -(bool)(exeOpc & 0x1));

				write_imageui(voxLitW, adrA, data);
			}
		}
	}
	
	/*const uint8_t osiz[16] = { 35, 7, 35, 7, 4, 6, 6, 6, 151, 7, 7, 11, 7, 11, 11, 15 };

	const uint8_t datBeg = comBuf[0];
	uint8_t opcAre = 1;

	uint2 idxRan = { 0, 0 };

	uint32_t addBeg = datBeg;

	uint8_t  exeOpc;
	uint32_t exeSiz;
	uint32_t exeIdx;
	uint32_t exeAdd;

	image2d_t* target;
	int2 address = { 0, 0 };
	int2 destination = { 0, 0 };
	uint4 data = { 0, 0, 0, 1 };

	while (opcAre < datBeg) {
		idxRan.lo = idxRan.hi;
		idxRan.hi += ((uint32_t)(comBuf[opcAre] & 0x0F) << 16) | ((uint32_t)comBuf[opcAre + 1] << 8) | comBuf[opcAre + 2];

		exeOpc = (comBuf[opcAre] & 0xF0) >> 4;
		exeSiz = osiz[exeOpc];

		exeIdx = atomic_inc(opcCnt);

		while (exeIdx < idxRan.hi) {
			exeAdd = addBeg + (exeIdx - idxRan.lo) * exeSiz;

			address = decodeAddress(uint24_b(comBuf, 0));

			if (exeOpc <= INS_UMT) {
				target = (exeOpc <= 0x1) ? &voxStrW : &voxMatW;

				address &= ~(((int2)(0x02, 0x00) ^ ((address.x & 0x02) ^ (address.y & 0x02))) | (exeOpc & 0x01));

				data.x = uint32_b(comBuf, 3);
				data.y = data.z = 0;

				write_imageui(*target, address, data);

				if (exeOpc & 0x01) {
					address.x |= 0x01;
					data.x = uint32_b(comBuf, 7);
					write_imageui(*target, address, data);

					address ^= 0x01;
					data.x = uint32_b(comBuf, 11);
					write_imageui(*target, address, data);

					address.x |= 0x01;
					data.x = uint32_b(comBuf, 15);
					write_imageui(*target, address, data);

					address ^= 0x01;
					address |= (int2)(0x02, 0x00) ^ ((address.x & 0x02) ^ (address.y & 0x02));
					data.x = uint32_b(comBuf, 19);
					write_imageui(*target, address, data);

					address.x |= 0x01;
					data.x = uint32_b(comBuf, 23);
					write_imageui(*target, address, data);

					address ^= 0x01;
					data.x = uint32_b(comBuf, 27);
					write_imageui(*target, address, data);

					address.x |= 0x01;
					data.x = uint32_b(comBuf, 31);
					write_imageui(*target, address, data);
				}
			}
			else if (exeOpc == INS_LLT) {
				// TO DO: Light loading
			}
			else if (exeOpc == INS_CPY) {
				destination = decodeAddress(uint24_b(comBuf, 3));

				data = read_imageui(voxStrR, voxSampler, address);
				write_imageui(voxStrW, destination, data);

				data = read_imageui(voxMatR, voxSampler, address);
				write_imageui(voxMatW, destination, data);
			}
			else if (exeOpc <= INS_UME) {
				data = read_imageui(voxStrR, voxSampler, address);

				data.x &= 0xFFFFFF00 ^ ((0xFFFFFF00 ^ 0x600000FF) & -(exeOpc != INS_UCH));
				data.x |= comBuf[exeAdd + 3] ^ ((comBuf[exeAdd + 3] ^ (((uint32_t)((exeOpc & 0x01) ^ 0x01) << 31) | (uint24_b(comBuf, 3) << 8))) & -(exeOpc != INS_UCH));

				write_imageui(voxStrW, address, data);
			}
			else {
				address = (int2)(((uint32_t)(comBuf[1] & 0x0F) << 8) | comBuf[2], ((uint32_t)comBuf[0] << 4) | (comBuf[1] >> 4));

				//data.x = data.x ^ ((data.x ^ uint32_b(comBuf, 0x3)) & -(bool)(exeOpc & 0x4));
				//data.y = data.y ^ ((data.y ^ uint32_b(comBuf, 0x3 | (exeOpc & 0x4))) & -(bool)(exeOpc & 0x2));
				//data.z = data.z ^ ((data.z ^ uint32_b(comBuf, 0x3 | ((exeOpc + 2) & 0xC))) & -(bool)(exeOpc & 0x1));

				data = select(read_imageui(voxLitR, voxSampler, address), (uint4)(uint32_b(comBuf, 0x3), uint32_b(comBuf, 0x3 | (exeOpc & 0x4)), uint32_b(comBuf, 0x3 | ((exeOpc + 2) & 0xC)), 1), (uint4)(exeOpc << 29, exeOpc << 30, exeOpc << 31, 0));
				
				write_imageui(voxLitW, address, data);
			}
		}

		addBeg += (idxRan.hi - idxRan.lo) * exeSiz;

		opcAre += 3;
	}*/
}