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

// intel codebuilder macro

#define IntelCodeBuilder 1

/*KERNEL_INCLUDE_END*/

// voxel depth constant

#define VoxelDepth 23


// function declarations

void cgLoad(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3);
void cgUnload(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent);
void cgAdd(__global uint32_t* vxBuffer, uint32_t parent, uint32_t ca_0, uint32_t ca_1, uint32_t ca_2, uint32_t ca_3, uint32_t cb_0, uint32_t cb_1, uint32_t cb_2, uint32_t cb_3);
void cgRemove(__global uint32_t* vxBuffer, uint32_t parent, uint8_t mask);
void cgMove(__global uint32_t* vxBuffer, uint32_t fparent, uint8_t fidx, uint32_t tparent, uint8_t tidx);
void cgExpand(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent, uint32_t index);
void cgColour(__global uint32_t* vxBuffer, uint32_t index, uint32_t colour_0, uint32_t colour_1);
void cgLight(__global uint32_t* vxBuffer, uint32_t index, uint32_t light);

uint32_t getParent(__global uint32_t* vxBuffer, uint32_t index);
uint32_t getTimestamp(__global uint32_t* vxBuffer, uint32_t index);

// kernels

__kernel void rayInitKernel(__global __write_only float* rvLookup, float pixWidth, float pixHeight, float halWidth, float halHeight, uint16_t width) {
	int2 pixel = { get_global_id(0), get_global_id(1) };
	
	float3 v = { pixel.x * pixWidth - halWidth, pixel.y * pixHeight - halHeight, -1.0f };

	vstore3(v / length(v), pixel.y * width + pixel.x, rvLookup);
}

__kernel void cgProKernel(__global __read_write uint32_t* vxBuffer, volatile __global __read_write uint32_t* mnTicket, volatile __global __read_write uint32_t* mnBuffer, __global __read_only uint8_t* cgBuffer, uint32_t syncInsAmount, uint32_t asyncInsAmount) {
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
			case INS_CTG_RLD_C: cgLoad(vxBuffer, mnTicket, mnBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32), INT_BUF(cgBuffer, pointer + 36), INT_BUF(cgBuffer, pointer + 40), INT_BUF(cgBuffer, pointer + 44), INT_BUF(cgBuffer, pointer + 48), INT_BUF(cgBuffer, pointer + 52), INT_BUF(cgBuffer, pointer + 56), INT_BUF(cgBuffer, pointer + 60), INT_BUF(cgBuffer, pointer + 64), INT_BUF(cgBuffer, pointer + 68), INT_BUF(cgBuffer, pointer + 72), INT_BUF(cgBuffer, pointer + 76), INT_BUF(cgBuffer, pointer + 80), INT_BUF(cgBuffer, pointer + 84), INT_BUF(cgBuffer, pointer + 88), INT_BUF(cgBuffer, pointer + 92), INT_BUF(cgBuffer, pointer + 96), INT_BUF(cgBuffer, pointer + 100), INT_BUF(cgBuffer, pointer + 104), INT_BUF(cgBuffer, pointer + 108), INT_BUF(cgBuffer, pointer + 112), INT_BUF(cgBuffer, pointer + 116), INT_BUF(cgBuffer, pointer + 120), INT_BUF(cgBuffer, pointer + 124), INT_BUF(cgBuffer, pointer + 128), INT_BUF(cgBuffer, pointer + 132)); break;
			case INS_CTG_ULD_C: cgUnload(vxBuffer, mnTicket, mnBuffer, INT_BUF(cgBuffer, pointer)); break;
			case INS_CTG_ADD_C: cgAdd(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), INT_BUF(cgBuffer, pointer + 8), INT_BUF(cgBuffer, pointer + 12), INT_BUF(cgBuffer, pointer + 16), INT_BUF(cgBuffer, pointer + 20), INT_BUF(cgBuffer, pointer + 24), INT_BUF(cgBuffer, pointer + 28), INT_BUF(cgBuffer, pointer + 32)); break;
			case INS_CTG_REM_C: cgRemove(vxBuffer, INT_BUF(cgBuffer, pointer), cgBuffer[pointer + 4]); break; break;
			case INS_CTG_MOV_C: cgMove(vxBuffer, INT_BUF(cgBuffer, pointer), ((cgBuffer[pointer + 8] & 0xE0) >> 5), INT_BUF(cgBuffer, pointer + 4), ((cgBuffer[pointer + 8] & 0x1C) >> 2)); break;
			case INS_CTG_EXP_C: cgExpand(vxBuffer, mnTicket, mnBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4)); break;
			case INS_CTG_COL_C: cgColour(vxBuffer, INT_BUF(cgBuffer, pointer), INT_BUF(cgBuffer, pointer + 4), ((uint32_t)cgBuffer[pointer + 8] << 24)); break;
			case INS_CTG_LIT_C: cgLight(vxBuffer, INT_BUF(cgBuffer, pointer), (((uint32_t)cgBuffer[pointer + 4] << 16) | ((uint32_t)cgBuffer[pointer + 5] << 8) | cgBuffer[pointer + 6])); break;
		}

		amount -= 1;
		pointer += size;
	}
}

/*

Raycasting algorithm based on CUDA implementation in "Efficient Sparse Voxel Octrees – Analysis, Extensions, and Implementation"
                                                  by Samuli Laine and Tero Karras of NVIDA Research

Colour model according to "A physically Based Colour Model"
                       by Robert J Oddy and Philip J Willis

*/

__kernel void renderKernel(volatile __global __read_write uint32_t* vxBuffer, volatile __global __read_write uint32_t* mnTicket, volatile __global __read_write uint32_t* mnBuffer, __global __read_write uint8_t* gcBuffer, __write_only image2d_t rbo, __global __read_only float* rvLookup, __constant __read_only float* rotMat, float3 norPos, float rayCoef, uint32_t timStamp) {
	int2 size = { get_image_width(rbo), get_image_height(rbo) };
	const int2 coors = { get_global_id(0), get_global_id(1) };

	float3 dir = vload3(coors.y * size.x + coors.x, rvLookup);

	dir = (float3)(rotMat[0] * dir.x + rotMat[1] * dir.y + rotMat[2] * dir.z,
		rotMat[3] * dir.x + rotMat[4] * dir.y + rotMat[5] * dir.z,
		rotMat[6] * dir.x + rotMat[7] * dir.y + rotMat[8] * dir.z);

	const float epsilon = as_float((127 - VoxelDepth) << 23);

	uint3 stack[VoxelDepth];

	if (fabs(dir.x) < epsilon) dir.x = as_float(as_uint(epsilon) ^ (as_uint(dir.x) & 0x80000000));
	if (fabs(dir.y) < epsilon) dir.y = as_float(as_uint(epsilon) ^ (as_uint(dir.y) & 0x80000000));
	if (fabs(dir.z) < epsilon) dir.z = as_float(as_uint(epsilon) ^ (as_uint(dir.z) & 0x80000000));

	uint8_t octant_mask = 0x00;

	if (dir.x > 0.0f) { octant_mask ^= 1; norPos.x = 3.0f - norPos.x; }
	if (dir.y > 0.0f) { octant_mask ^= 2; norPos.y = 3.0f - norPos.y; }
	if (dir.z > 0.0f) { octant_mask ^= 4; norPos.z = 3.0f - norPos.z; }

	float tx_coef = 1.0f / -fabs(dir.x);
	float ty_coef = 1.0f / -fabs(dir.y);
	float tz_coef = 1.0f / -fabs(dir.z);

	float tx_bias = tx_coef * norPos.x;
	float ty_bias = ty_coef * norPos.y;
	float tz_bias = tz_coef * norPos.z;

	float t_min = fmax(fmax(2.0f * tx_coef - tx_bias, 2.0f * ty_coef - ty_bias), 2.0f * tz_coef - tz_bias);
	float t_max = fmin(fmin(tx_coef - tx_bias, ty_coef - ty_bias), tz_coef - tz_bias);
	
	float h = t_max;

	t_min = fmax(t_min, 0.0f);
	t_max = fmin(t_max, 1.0f);

	volatile __global uint32_t* parent = vxBuffer;
	uint4 voxel = { 0x00, 0x00, 0x00, 0x00 };

	uint8_t idx = 0x00;

	float3 pos = { 1.0f, 1.0f, 1.0f };

	uint8_t scale = VoxelDepth - 1;
	float scaExp2 = 0.5f;

	if ((norPos.x - 1.5f) > t_min) { idx ^= 1; pos.x = 1.5f; }
	if ((norPos.y - 1.5f) > t_min) { idx ^= 2; pos.y = 1.5f; }
	if ((norPos.z - 1.5f) > t_min) { idx ^= 4; pos.z = 1.5f; }
	
	float tx_corner, ty_corner, tz_corner, tc_max;
	uint8_t cidx;

	timStamp = (timStamp & 0x0000003F) << 10;

	uint32_t oldTimeStamp;

	uint32_t ticketIndex;

	bool shouldRender = false;
	bool hasRequested = false;
	uint8_t step_mask = 0x00;

	uint32_t difBits;

	uint32_t shx, shy, shz;

	bool prev_active = false, prev_emitter = false;
	bool curr_active, curr_transparent, curr_emitter;

	float3 curr_light, fron_light;
	float3* prev_light = &curr_light;
	float3 back_light = { 0.0f, 0.0f, 0.0f };

	float3 sqrt_light, corr_light;
	uchar4 dist_light;

	float3 fac = { 1.0f, 1.0f, 1.0f };
	float3 buf;

	float8 material;

	float curr_depth = 0.0f;
	uint8_t curr_side = 0x00;

	float4 pixel = { 0.0f, 0.0f, 0.0f, 1.0f };

	while (scale < VoxelDepth) {
		if ((voxel.x & 0x80000000) == 0) {
			voxel = *((__global uint4*)parent);
		}

		tx_corner = pos.x * tx_coef - tx_bias;
		ty_corner = pos.y * ty_coef - ty_bias;
		tz_corner = pos.z * tz_coef - tz_bias;

		tc_max = fmin(fmin(tx_corner, ty_corner), tz_corner);

		cidx = idx ^ octant_mask;

		shouldRender = (!(voxel.x & 0x00FF0000) || (tc_max > (scaExp2 * rayCoef)));
		
		if (!shouldRender && ((voxel.x & (0x00800000 >> cidx)) != 0x00) && (t_min <= t_max)) {
			if (((voxel.x & 0x0000FC00) != timStamp) && (!hasRequested || (voxel.y != 0x00))) {
				oldTimeStamp = atomic_cmpxchg(parent, voxel.x, (voxel.x & 0xFFFF03FF) | timStamp);

				if (oldTimeStamp == voxel.x) {
					ticketIndex = atomic_inc(mnTicket);

					while (mnTicket[0x01] != ticketIndex) {
						continue;
					}

					if (voxel.y == 0x00) {
						ticketIndex = ((gcBuffer[15] << 16) | (gcBuffer[16] << 8) | gcBuffer[17]);

						gcBuffer[21 + (ticketIndex << 2) + 0] = (((parent - vxBuffer) >> 2) & 0xFF000000) >> 24;
						gcBuffer[21 + (ticketIndex << 2) + 1] = (((parent - vxBuffer) >> 2) & 0x00FF0000) >> 16;
						gcBuffer[21 + (ticketIndex << 2) + 2] = (((parent - vxBuffer) >> 2) & 0x0000FF00) >> 8;
						gcBuffer[21 + (ticketIndex << 2) + 3] = (((parent - vxBuffer) >> 2) & 0x000000FF);

						ticketIndex += 1;

						gcBuffer[15] = (ticketIndex & 0x00FF0000) >> 16;
						gcBuffer[16] = (ticketIndex & 0x0000FF00) >> 8;
						gcBuffer[17] = ticketIndex & 0x000000FF;

#if OpenCLDebug
						printf("gcRequest(parent: %u)\n\n", (parent - vxBuffer) >> 2);
#endif
					}
					else {
						ticketIndex = (voxel.y >> 3);
						
						mnBuffer[(mnBuffer[ticketIndex << 1] << 1) | 0x01] = mnBuffer[(ticketIndex << 1) | 0x01];
						mnBuffer[mnBuffer[(ticketIndex << 1) | 0x01] << 1] = mnBuffer[ticketIndex << 1];

						mnBuffer[(ticketIndex << 1) | 0x01] = 0x00;
						mnBuffer[ticketIndex << 1] = mnBuffer[0x00];
						
						mnBuffer[(mnBuffer[0x00] << 1) | 0x01] = ticketIndex;
						mnBuffer[0x00] = ticketIndex;

#if OpenCLDebug
						printf("gcValidate(parent: %u)\n\n", (parent - vxBuffer) >> 2);
#endif
					}

					mnTicket[0x01] += 1;
				}
			}

			if (voxel.y == 0x00) {
				shouldRender = true;

				voxel.x &= 0xFF00FFFF;
			}
			else {
				if (tc_max < h) {
					stack[scale].xy = (uint2)((parent - vxBuffer) >> 2, as_uint(t_max));
				}

				stack[scale].z = ((as_uint((tc_max - t_min) / scaExp2) & 0xFFFFFFF8) | (step_mask & 0x07));

				h = tc_max;

				parent = vxBuffer + ((voxel.y + cidx) << 2);

				idx = 0x00;
				scale--;
				scaExp2 *= 0.5f;

				if ((scaExp2 * tx_coef + tx_corner) > t_min) { idx ^= 1; pos.x += scaExp2; }
				if ((scaExp2 * ty_coef + ty_corner) > t_min) { idx ^= 2; pos.y += scaExp2; }
				if ((scaExp2 * tz_coef + tz_corner) > t_min) { idx ^= 4; pos.z += scaExp2; }

				t_max = fmin(t_max, tc_max);
				voxel.x = 0x00;

				continue;
			}
		}

		step_mask = 0x00;

		if (tx_corner <= tc_max) { step_mask ^= 1; pos.x -= scaExp2; }
		if (ty_corner <= tc_max) { step_mask ^= 2; pos.y -= scaExp2; }
		if (tz_corner <= tc_max) { step_mask ^= 4; pos.z -= scaExp2; }

		t_min = tc_max;
		idx ^= step_mask;

		if (shouldRender || ((idx & step_mask) != 0x00)) {
			curr_active = (voxel.x & 0x80000000);
			curr_emitter = ((voxel.w & 0x01000000) >> 24);

			curr_light = (float3)(((voxel.w & 0x00FC0000) >> 18) / 63.0f, ((voxel.w & 0x0003F000) >> 12) / 63.0f, ((voxel.w & 0x00000FC0) >> 6) / 63.0f);

			if ((prev_active || curr_active) && ((voxel.x & 0x00FF0000) == 0x00 || (t_min > t_max && tc_max <= (scaExp2 * rayCoef)))) {
				material = (float8)(((voxel.z & 0xFC000000) >> 26) / 63.0f, ((voxel.z & 0x03F00000) >> 20) / 63.0f, ((voxel.z & 0x000FC000) >> 14) / 63.0f,
					((voxel.z & 0x00003F00) >> 8) / 63.0f,
					((voxel.z & 0x000000F8) >> 3) / 31.0f, (((voxel.z & 0x00000007) << 2) | ((voxel.w & 0xC0000000) >> 30)) / 31.0f, ((voxel.w & 0x3E000000) >> 25) / 31.0f,
					((voxel.w & 0x01000000) >> 24) / 1.0f
				);

				curr_transparent = (material.s3 < 1.0f);

				sqrt_light = (float3)(native_sqrt((prev_light->x * prev_light->x + as_float(as_uint(curr_light.x * curr_light.x) & -curr_transparent)) / 2.0f),
					native_sqrt((prev_light->y * prev_light->y + as_float(as_uint(curr_light.y * curr_light.y) & -curr_transparent)) / 2.0f),
					native_sqrt((prev_light->z * prev_light->z + as_float(as_uint(curr_light.z * curr_light.z) & -curr_transparent)) / 2.0f)
				);

				curr_side = (stack[scale + 1].z & 0x06) + (bool)((octant_mask ^ 0x07) & stack[scale + 1].z);
				
				dist_light = (uchar4)((bool)(voxel.w & (0x00800000 >> curr_side)), (bool)(voxel.w & (0x00020000 >> curr_side)), (bool)(voxel.w & (0x00000800 >> curr_side)), (bool)(voxel.w & (0x00000020 >> curr_side)));
				corr_light = convert_float3(dist_light.xyz) - (convert_float3(dist_light.xyz) - (*prev_light)) * as_float(0x3E000000 | ((!dist_light.w || !(dist_light.x || dist_light.y || dist_light.z)) << 24) | ((!((dist_light.x || dist_light.y || dist_light.z) ^ dist_light.w)) << 23));

				if (prev_active) {
					back_light = (float3)(as_float((0x3F800000 & -prev_emitter) | (as_uint(sqrt_light.x) & -(!prev_emitter && (curr_transparent || !curr_emitter))) | (as_uint(corr_light.x) & -(!prev_emitter && !curr_transparent && curr_emitter))),
						as_float((0x3F800000 & -prev_emitter) | (as_uint(sqrt_light.y) & -(!prev_emitter && (curr_transparent || !curr_emitter))) | (as_uint(corr_light.y) & -(!prev_emitter && !curr_transparent && curr_emitter))),
						as_float((0x3F800000 & -prev_emitter) | (as_uint(sqrt_light.z) & -(!prev_emitter && (curr_transparent || !curr_emitter))) | (as_uint(corr_light.z) & -(!prev_emitter && !curr_transparent && curr_emitter)))
					);
				}

				if (curr_active) {
					buf = (float3)(as_float((0x3F800000 & -curr_emitter) | (as_uint(sqrt_light.x) & -(!curr_emitter && curr_transparent)) | (as_uint(corr_light.x) & -(!curr_emitter && !curr_transparent))),
						as_float((0x3F800000 & -curr_emitter) | (as_uint(sqrt_light.y) & -(!curr_emitter && curr_transparent)) | (as_uint(corr_light.y) & -(!curr_emitter && !curr_transparent))),
						as_float((0x3F800000 & -curr_emitter) | (as_uint(sqrt_light.z) & -(!curr_emitter && curr_transparent)) | (as_uint(corr_light.z) & -(!curr_emitter && !curr_transparent)))
					);

					buf = buf * material.s012 + back_light / 3.0f;

					back_light = (float3)(0.0f, 0.0f, 0.0f);

					buf /= fmax(1.0f, fmax(buf.x, fmax(buf.y, buf.z)));

					pixel += (float4)(buf * fac * material.s3, 0.0f);

					curr_depth = as_float(stack[scale + 1].z & 0xFFFFFFF8);

					fac *= (float3)(native_powr(material.s4 + 0.00390625f, curr_depth), native_powr(material.s5 + 0.00390625f, curr_depth), native_powr(material.s6 + 0.00390625f, curr_depth)) * (1.0f - material.s3);

					if (fac.x <= 0.00390625 && fac.y <= 0.00390625 && fac.z <= 0.00390625) {
						break;
					}
				}
			}

			prev_active = curr_active;
			prev_emitter = curr_emitter;

			fron_light = curr_light;
			prev_light = &fron_light;

			difBits = 0x00;

			if ((step_mask & 0x01) != 0x00) difBits |= as_uint(pos.x) ^ as_int(pos.x + scaExp2);
			if ((step_mask & 0x02) != 0x00) difBits |= as_uint(pos.y) ^ as_int(pos.y + scaExp2);
			if ((step_mask & 0x04) != 0x00) difBits |= as_uint(pos.z) ^ as_int(pos.z + scaExp2);

			scale = (as_int((float)difBits) >> 23) - 127;
			scaExp2 = as_float((127 - (VoxelDepth - scale)) << 23);
			
			shx = as_uint(pos.x) >> scale;
			shy = as_uint(pos.y) >> scale;
			shz = as_uint(pos.z) >> scale;

			pos.x = as_float(shx << scale);
			pos.y = as_float(shy << scale);
			pos.z = as_float(shz << scale);

			idx = (shx & 0x01) | ((shy & 0x01) << 1) | ((shz & 0x01) << 2);

			parent = vxBuffer + (stack[scale].x << 2);
			t_max = as_float(stack[scale].y);

			h = 0.0f;
			voxel.x = 0x00;
		}
	}

	pixel += (float4)(back_light * fac, 1.0f);

	if (scale >= VoxelDepth) {
		t_min = 2.0f;
	}

	size /= 2;

	if ((coors.x >= (size.x - 5) && coors.x <= (size.x + 4) && coors.y >= (size.y - 1) && coors.y <= size.y) || (coors.y >= (size.y - 5) && coors.y <= (size.y + 4) && coors.x >= (size.x - 1) && coors.x <= size.x)) {
		pixel = (float4)((float)(pixel.x < 0.5f), (float)(pixel.y < 0.5f), (float)(pixel.z < 0.5f), 1.0f);
	}

	write_imagef(rbo, coors, pixel);
}

__kernel void gcReqKernel(volatile __global __read_write uint32_t* vxBuffer, __global __read_write uint32_t* mnTicket, __global __read_write uint32_t* mnBuffer, __global __read_write uint8_t* gcBuffer, uint32_t timStamp, float memPressure) {
	mnTicket[0x00] = mnTicket[0x01] = 0x00;

	uint32_t timeDiff = ((getTimestamp(vxBuffer, getParent(vxBuffer, mnBuffer[0x01] << 3)) - (timStamp &= 0x0000003F)) & 0x0000003F);

	bool hasSuggestions = ((timeDiff != 0) && (timeDiff <= (uint8_t)round(native_powr(63.0f, native_sqrt(clamp(memPressure, 0.0f, 1.0f))))));
	bool hasRequests = (gcBuffer[15] || gcBuffer[16] || gcBuffer[17]);

	if (!hasRequests && !hasSuggestions) {
		gcBuffer[0] = 0xFF;

		gcBuffer[1] = gcBuffer[2] = gcBuffer[3] = gcBuffer[4] = gcBuffer[5] = gcBuffer[6] = 0x00;

		return;
	}

	uint32_t byteSize;

	if (!hasSuggestions) {
		gcBuffer[0] = 0xFF;

		byteSize = 17 + ((gcBuffer[15] << 16) | (gcBuffer[16] << 8) | gcBuffer[17]) * INS_GTC_REQ_S;
		
		gcBuffer[1] = (byteSize & 0x00FF0000) >> 16;
		gcBuffer[2] = (byteSize & 0x0000FF00) >> 8;
		gcBuffer[3] = (byteSize & 0x000000FF);

		gcBuffer[4] = gcBuffer[8] = gcBuffer[15];
		gcBuffer[5] = gcBuffer[9] = gcBuffer[16];
		gcBuffer[6] = gcBuffer[10] = gcBuffer[17];

		gcBuffer[7] = INS_GTC_REQ_C;

		gcBuffer[11] = gcBuffer[12] = 0x00;
		gcBuffer[13] = 17;

		return;
	}

	gcBuffer[0] = 0x00;

	gcBuffer[7] = INS_GTC_SUG_C;

	if (hasRequests) {
		byteSize = 17 + ((gcBuffer[15] << 16) | (gcBuffer[16] << 8) | gcBuffer[17]) * INS_GTC_REQ_S;

		gcBuffer[4] = gcBuffer[15];
		gcBuffer[5] = gcBuffer[16];
		gcBuffer[6] = gcBuffer[17];

		gcBuffer[14] = INS_GTC_REQ_C;

		gcBuffer[18] = gcBuffer[19] = 0x00;
		gcBuffer[20] = 17;
	}
	else {
		byteSize = 10;

		gcBuffer[4] = gcBuffer[5] = gcBuffer[6] = 0x00;
	}

	gcBuffer[1] = gcBuffer[11] = (byteSize & 0x00FF0000) >> 16;
	gcBuffer[2] = gcBuffer[12] = (byteSize & 0x0000FF00) >> 8;
	gcBuffer[3] = gcBuffer[13] = (byteSize & 0x000000FF);
}

__kernel void gcSugKernel(volatile __global __read_write uint32_t* vxBuffer, volatile __global __read_write uint32_t* mnTicket, volatile __global __read_write uint32_t* mnBuffer, volatile __global __read_write uint8_t* gcBuffer, uint32_t timStamp, float memPressure) {
	uint32_t ticketIndex = 0;

	uint32_t preIndex, sugIndex;

	do {
		preIndex = mnTicket[0x00];
		sugIndex = mnBuffer[(preIndex << 1) | 0x01];

		ticketIndex += 1;
	} while (atomic_cmpxchg(mnTicket, preIndex, sugIndex) != preIndex);

	ticketIndex -= 1;

	preIndex = getParent(vxBuffer, sugIndex << 3);
	uint32_t timeDiff = ((getTimestamp(vxBuffer, preIndex) - (timStamp &= 0x0000003F)) & 0x0000003F);

	uint32_t memIndex = ((gcBuffer[11] << 16) | (gcBuffer[12] << 8) | gcBuffer[13]) + 4 + (((gcBuffer[8] << 16) | (gcBuffer[9] << 8) | gcBuffer[10]) + ticketIndex) * INS_GTC_SUG_S;
	
#ifndef IntelCodeBuilder
	bool suited = ((sugIndex != 0x00) && (timeDiff != 0) && ((memIndex + INS_GTC_SUG_S) <= (0x01 << 24)) && (timeDiff <= (uint8_t)round(native_powr(63.0f, native_sqrt(clamp(memPressure, 0.0f, 1.0f))))));

	atomic_max((__global uint32_t*)(mnTicket + 1), ticketIndex & -suited);

	atomic_add((__global uint32_t*)(mnTicket + 1), ((ticketIndex == 0x00) && suited));
#endif

	barrier(CLK_GLOBAL_MEM_FENCE);

	if (ticketIndex < mnTicket[1]) {
		gcBuffer[memIndex + 0] = (preIndex & 0xFF000000) >> 24;
		gcBuffer[memIndex + 1] = (preIndex & 0x00FF0000) >> 16;
		gcBuffer[memIndex + 2] = (preIndex & 0x0000FF00) >> 8;
		gcBuffer[memIndex + 3] = (preIndex & 0x000000FF);

		vxBuffer[preIndex << 2] = ((vxBuffer[preIndex << 2] & 0xFFFF03FF) | (timStamp << 10));
	}
	else {
		gcBuffer[0] = 0xFF;
	}

	if (ticketIndex == (mnTicket[0x01] - 1)) {
		preIndex = mnBuffer[0x01];

		mnBuffer[0x01] = mnBuffer[(sugIndex << 1) | 0x01];
		mnBuffer[mnBuffer[(sugIndex << 1) | 0x01] << 1] = 0x00;

		mnBuffer[(sugIndex << 1) | 0x01] = 0x00;
		mnBuffer[preIndex << 1] = mnBuffer[0x00];

		mnBuffer[(mnBuffer[0x00] << 1) | 0x01] = preIndex;
		mnBuffer[0x00] = sugIndex;

		sugIndex = ((gcBuffer[1] << 16) | (gcBuffer[2] << 8) | gcBuffer[3]) + (ticketIndex + 1) * INS_GTC_SUG_S;
		
		gcBuffer[1] = (sugIndex & 0x00FF0000) >> 16;
		gcBuffer[2] = (sugIndex & 0x0000FF00) >> 8;
		gcBuffer[3] = (sugIndex & 0x000000FF);

		sugIndex = ((gcBuffer[4] << 16) | (gcBuffer[5] << 8) | gcBuffer[6]) + ticketIndex + 1;

		gcBuffer[4] = (sugIndex & 0x00FF0000) >> 16;
		gcBuffer[5] = (sugIndex & 0x0000FF00) >> 8;
		gcBuffer[6] = (sugIndex & 0x000000FF);

		sugIndex = ((gcBuffer[8] << 16) | (gcBuffer[9] << 8) | gcBuffer[10]) + ticketIndex + 1;

		gcBuffer[8] = (sugIndex & 0x00FF0000) >> 16;
		gcBuffer[9] = (sugIndex & 0x0000FF00) >> 8;
		gcBuffer[10] = (sugIndex & 0x000000FF);
	}

	barrier(CLK_GLOBAL_MEM_FENCE);

	mnTicket[0x00] = mnTicket[0x01] = 0x00;
}


// functions

void cgLoad(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent, uint32_t index, uint32_t c0_0, uint32_t c0_1, uint32_t c0_2, uint32_t c0_3, uint32_t c1_0, uint32_t c1_1, uint32_t c1_2, uint32_t c1_3, uint32_t c2_0, uint32_t c2_1, uint32_t c2_2, uint32_t c2_3, uint32_t c3_0, uint32_t c3_1, uint32_t c3_2, uint32_t c3_3, uint32_t c4_0, uint32_t c4_1, uint32_t c4_2, uint32_t c4_3, uint32_t c5_0, uint32_t c5_1, uint32_t c5_2, uint32_t c5_3, uint32_t c6_0, uint32_t c6_1, uint32_t c6_2, uint32_t c6_3, uint32_t c7_0, uint32_t c7_1, uint32_t c7_2, uint32_t c7_3) {
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

	uint32_t ticketIndex = atomic_inc(mnTicket);

	while (mnTicket[0x01] != ticketIndex) {
		continue;
	}

	ticketIndex = (index >> 3);

	mnBuffer[(ticketIndex << 1) | 0x01] = 0x00;
	mnBuffer[ticketIndex << 1] = mnBuffer[0x00];

	mnBuffer[(mnBuffer[0x00] << 1) | 0x01] = ticketIndex;
	mnBuffer[0x00] = ticketIndex;

	mnTicket[0x01] += 1;
	
#if OpenCLDebug
	printf("cgLoad(parent: %u, index: %u)\n   ptr: %u\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, index, vxBuffer[(parent << 2) + 1], ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
#endif
}

void cgUnload(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent) {
	uint32_t children = vxBuffer[(parent << 2) + 1];
	
	vxBuffer[(parent << 2) + 1] = 0x00;

	uint32_t ticketIndex = atomic_inc(mnTicket);

	while (mnTicket[0x01] != ticketIndex) {
		continue;
	}

	ticketIndex = (children >> 3);

	mnBuffer[(mnBuffer[ticketIndex << 1] << 1) | 0x01] = mnBuffer[(ticketIndex << 1) | 0x01];
	mnBuffer[mnBuffer[(ticketIndex << 1) | 0x01] << 1] = mnBuffer[ticketIndex << 1];

	mnTicket[0x01] += 1;

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
		printf("cgAdd(parent: %u, aidx: %u, bidx: %u)\n   chm: 0x%02X\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, adx >> 2, bdx >> 2, (vxBuffer[parent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
	}
	else {
		printf("cgAdd(parent: %u, aidx: %u)\n   chm: 0x%02X\n   c_0: 0x%016llX%016llX\n   c_1: 0x%016llX%016llX\n   c_2: 0x%016llX%016llX\n   c_3: 0x%016llX%016llX\n   c_4: 0x%016llX%016llX\n   c_5: 0x%016llX%016llX\n   c_6: 0x%016llX%016llX\n   c_7: 0x%016llX%016llX\n\n", parent, adx >> 2, (vxBuffer[parent << 2] & 0x00FF0000) >> 16, ((uint64_t)vxBuffer[idx + 0] << 32) | vxBuffer[idx + 1], ((uint64_t)vxBuffer[idx + 2] << 32) | vxBuffer[idx + 3], ((uint64_t)vxBuffer[idx + 4] << 32) | vxBuffer[idx + 5], ((uint64_t)vxBuffer[idx + 6] << 32) | vxBuffer[idx + 7], ((uint64_t)vxBuffer[idx + 8] << 32) | vxBuffer[idx + 9], ((uint64_t)vxBuffer[idx + 10] << 32) | vxBuffer[idx + 11], ((uint64_t)vxBuffer[idx + 12] << 32) | vxBuffer[idx + 13], ((uint64_t)vxBuffer[idx + 14] << 32) | vxBuffer[idx + 15], ((uint64_t)vxBuffer[idx + 16] << 32) | vxBuffer[idx + 17], ((uint64_t)vxBuffer[idx + 18] << 32) | vxBuffer[idx + 19], ((uint64_t)vxBuffer[idx + 20] << 32) | vxBuffer[idx + 21], ((uint64_t)vxBuffer[idx + 22] << 32) | vxBuffer[idx + 23], ((uint64_t)vxBuffer[idx + 24] << 32) | vxBuffer[idx + 25], ((uint64_t)vxBuffer[idx + 26] << 32) | vxBuffer[idx + 27], ((uint64_t)vxBuffer[idx + 28] << 32) | vxBuffer[idx + 29], ((uint64_t)vxBuffer[idx + 30] << 32) | vxBuffer[idx + 31]);
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

void cgExpand(__global uint32_t* vxBuffer, volatile __global uint32_t* mnTicket, volatile __global uint32_t* mnBuffer, uint32_t parent, uint32_t index) {
	uint32_t idx = index << 2;
	
	vxBuffer[idx + 0] = (0x00000000 | (((vxBuffer[(parent - ((vxBuffer[parent << 2] & 0x70000000) >> 28)) << 2] & 0x0F000000) + 0x01000000) & -(parent != 0x00))); vxBuffer[idx + 1] = 0x00; vxBuffer[idx + 2] = 0x00; vxBuffer[idx + 3] = 0x00;
	vxBuffer[idx + 4] = (0x10000000 | (parent & 0x0F000000)); vxBuffer[idx + 5] = 0x00; vxBuffer[idx + 6] = 0x00; vxBuffer[idx + 7] = 0x00;
	vxBuffer[idx + 8] = (0x20000000 | ((parent & 0x00F00000) << 4)); vxBuffer[idx + 9] = 0x00; vxBuffer[idx + 10] = 0x00; vxBuffer[idx + 11] = 0x00;
	vxBuffer[idx + 12] = (0x30000000 | ((parent & 0x000F0000) << 8)); vxBuffer[idx + 13] = 0x00; vxBuffer[idx + 14] = 0x00; vxBuffer[idx + 15] = 0x00;
	vxBuffer[idx + 16] = (0x40000000 | ((parent & 0x0000F000) << 12)); vxBuffer[idx + 17] = 0x00; vxBuffer[idx + 18] = 0x00; vxBuffer[idx + 19] = 0x00;
	vxBuffer[idx + 20] = (0x50000000 | ((parent & 0x00000F00) << 16)); vxBuffer[idx + 21] = 0x00; vxBuffer[idx + 22] = 0x00; vxBuffer[idx + 23] = 0x00;
	vxBuffer[idx + 24] = (0x60000000 | ((parent & 0x000000F0) << 20)); vxBuffer[idx + 25] = 0x00; vxBuffer[idx + 26] = 0x00; vxBuffer[idx + 27] = 0x00;
	vxBuffer[idx + 28] = (0x70000000 | ((parent & 0x0000000F) << 24)); vxBuffer[idx + 29] = 0x00; vxBuffer[idx + 30] = 0x00; vxBuffer[idx + 31] = 0x00;

	vxBuffer[(parent << 2) + 1] = index;

	uint32_t ticketIndex = atomic_inc(mnTicket);

	while (mnTicket[0x01] != ticketIndex) {
		continue;
	}

	ticketIndex = (index >> 3);

	mnBuffer[(ticketIndex << 1) | 0x01] = 0x00;
	mnBuffer[ticketIndex << 1] = mnBuffer[0x00];

	mnBuffer[(mnBuffer[0x00] << 1) | 0x01] = ticketIndex;
	mnBuffer[0x00] = ticketIndex;

	mnTicket[0x01] += 1;
	
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


uint32_t getParent(__global uint32_t* vxBuffer, uint32_t index) {
	index <<= 2;
	
	return ((vxBuffer[index + 4] & 0x0F000000) | ((vxBuffer[index + 8] & 0x0F000000) >> 4) | ((vxBuffer[index + 12] & 0x0F000000) >> 8) | ((vxBuffer[index + 16] & 0x0F000000) >> 12) |
		    ((vxBuffer[index + 20] & 0x0F000000) >> 16) | ((vxBuffer[index + 24] & 0x0F000000) >> 20) | ((vxBuffer[index + 28] & 0x0F000000) >> 24));
}

uint32_t getTimestamp(__global uint32_t* vxBuffer, uint32_t index) {
	return ((vxBuffer[index << 2] & 0x0000FC00) >> 10);
}