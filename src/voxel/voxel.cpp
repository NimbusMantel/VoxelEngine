#include "voxel.hpp"

#include <assert.h>

#define ceil(n) (int)(n + 0.5f)

static const uint32_t bpow = (0x01 << BUFFER_DEPTH);

static uint32_t manBuffer[ceil((0xFFFFFFFF >> (31 - BUFFER_DEPTH)) / 32.0f)];

static uint32_t space = bpow;

#if BUFFER_SANITY
#define scheck(p) assert(p < bpow);
#else
#define scheck(p) 
#endif

#define mget(p) (manBuffer[p >> 5] & (0x01 << (p & 0x1F)))
#define mset(p, b) manBuffer[p >> 5] = (manBuffer[p >> 5] & (0xFFFFFFFF ^ (0x01 << (p & 0x1F)))) | (((uint32_t)b) << (p & 0x1F))

void als(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec, uint32_t srt) {
	uint8_t level;
	
	if (srt) {
		for (level = BUFFER_DEPTH; level > 0; --level) {
			if ((srt & 0x01) && !mget(srt)) {
				srt += 1;

				break;
			}

			srt >> 1;
		}

		if (level == 0) return;
	}
	else {
		srt = 0x02;

		level = 1;
	}

	for (; level <= BUFFER_DEPTH; ++level) {
		srt = (srt + ((bool)mget(srt - 1))) << 1;
	}

	uint32_t free = mgec(srt, siz);

	vec.push_back(std::make_pair(srt, free));

	if (free != siz) return;
	
	als(siz - free, vec, srt + free);
}

uint32_t mgec(uint32_t pos, uint32_t siz) {
	// manBuf chunk get TO DO
}

namespace manBuf {
	uint32_t spa() {
		return space;
	}

	bool get(uint32_t pos) {
		scheck(pos);
		return (bool)mget((pos | bpow) - 1);
	}

	void set(uint32_t pos, bool tog) {
		scheck(pos);

		pos |= bpow;

		bool prev = (bool)mget(pos - 1);

		if (tog == prev) return;

		mset(pos - 1, tog);

		space += ((-tog) | 0x01);

		prev = !prev;

		bool sib;

		for (uint8_t inv = 1; inv <= BUFFER_DEPTH; ++inv) {
			sib = (bool)mget(((pos & 0xFFFFFFFE) | ((pos & 0x01) ^ 0x01)) - 1);

			pos >>= 1;

			if (((bool)mget(pos - 1)) == (prev && sib)) break;

			prev = (prev && sib);

			mset(pos - 1, prev);
		}
	}

	void alo(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec) {
		if (mget(0)) return;

		als(siz, vec, 0x00);
	}
}