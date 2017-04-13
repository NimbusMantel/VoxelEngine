#include "voxel.hpp"

#include <assert.h>

#define S1(p) (p | (p >> 1))
#define S2(p) (S1(p) | (S1(p) >> 2))
#define S4(p) (S2(p) | (S2(p) >> 4))
#define S8(p) (S4(p) | (S4(p) >> 8))
#define S16(p) (S8(p) | (S8(p) >> 16))

#define ceil(n) (int)(n + 0.5f)

static const uint32_t bpow = (0x01 << BUFFER_DEPTH);

static uint32_t manBuffer[ceil(S16(bpow) / 32.0f)];

#define mget(p) (manBuffer[p >> 5] & (0x00000001 << (p & 0x1F)))
#define mset(p, b) manBuffer[p >> 5] = (manBuffer[p >> 5] & (0xFFFFFFFF ^ (0x00000001 << (p & 0x1F)))) | (((uint32_t)b) << (p & 0x1F))

#if BUFFER_SANITY
#define scheck(p) assert(p < bpow);
#else
#define scheck(p) 
#endif

namespace manBuf {
	bool get(uint32_t pos) {
		scheck(pos);
		return (bool)mget((pos | bpow) - 2);
	}

	void set(uint32_t pos, bool toggled) {
		scheck(pos);
		mset((pos | bpow) - 2, toggled);
	}
}