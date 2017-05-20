#include "voxel.hpp"

#include <assert.h>
#include <intrin.h>
#include <iostream>
#include <bitset>

#define ceil(n) ((int)(n) + (n > (int)(n)))

static uint32_t __inline clz(uint32_t n) {
	unsigned long r; _BitScanForward(&r, n); return r;
}

static uint32_t __inline ctz(uint32_t n) {
	unsigned long r; _BitScanReverse(&r, n); return r;
}

static uint32_t __inline cnz(uint32_t n) {
	return (32 - __popcnt(n));
}

static const uint32_t bpow = (0x01 << BUFFER_DEPTH);

static uint32_t manBuffer[ceil((0xFFFFFFFF >> (31 - BUFFER_DEPTH)) / 32.0f)];

static uint32_t space = bpow;

#if BUFFER_SANITY
#define scheck(p) assert(p < bpow);
#define zcheck(p) assert(p != 0);
#define ocheck(p, a) if (p == 1) {a}
#else
#define scheck(p) 
#define zcheck(p) 
#define ockeck(p, a) 
#endif

#define mget(p) (manBuffer[p >> 5] & (0x01 << (p & 0x1F)))
#define mset(p, b) manBuffer[p >> 5] = (manBuffer[p >> 5] & (0xFFFFFFFF ^ (0x01 << (p & 0x1F)))) | (((uint32_t)b) << (p & 0x1F))

uint32_t mgec(uint32_t pos, uint32_t siz) {
	uint32_t free = 0;

	if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
		if (!((manBuffer[pos >> 5] << (pos & 0x1F)) >> (32 - siz))) {
			return siz;
		}
		else {
			
			return clz(manBuffer[pos >> 5] << (pos & 0x1F));
		}
	}

	if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
		if (!(manBuffer[pos >> 5] << (pos & 0x1F))) {
			free += (32 - (pos & 0x1F));
			pos += free;
			siz -= free;
		}
		else {
			return clz(manBuffer[pos >> 5] << (pos & 0x1F));
		}
	}

	for (; siz >= 32; siz -= 32) {
		if (!manBuffer[pos >> 5]) {
			pos += 32;
		}
		else {
			free += clz(manBuffer[pos >> 5]);

			return free;
		}
	}

	if (siz > 0) {
		if (!(manBuffer[pos >> 5] >> (32 - siz))) {
			free += (32 - siz);
		}
		else {
			free += clz(manBuffer[pos >> 5]);
		}
	}

	return free;
}

void mses(uint32_t pos, bool tog) {
	bool prev = (bool)mget(pos);

	if (tog == prev) return;

	mset(pos, tog);

	space += ((-tog) | 0x01);

	prev = !prev;

	bool sib;

	for (uint8_t inv = 1; inv <= BUFFER_DEPTH; ++inv) {
		sib = (bool)mget(pos + ((-((pos & 0x01) ^ 0x01)) | 0x01));
		
		pos = (pos - ((pos & 0x01) ^ 0x01)) >> 1;

		if (((bool)mget(pos)) == (prev && sib)) break;

		prev = (prev && sib);

		mset(pos, prev);
	}
}

void mseo(uint32_t pos, uint32_t siz) {
	uint32_t spos = pos;
	uint32_t epos = (pos + siz - 1);
	
	if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
		space -= (cnz((manBuffer[pos >> 5] << (pos & 0x1F)) >> (32 - siz)) + siz - 32);

		manBuffer[pos >> 5] |= ((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
	}
	else {
		if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
			space -= (cnz(manBuffer[pos >> 5] << (pos & 0x1F)) - (pos & 0x1F));

			manBuffer[pos >> 5] |= (0xFFFFFFFF >> (pos & 0x1F));

			pos += (32 - (pos & 0x1F));
		}

		for (; siz >= 32; siz -= 32) {
			space -= cnz(manBuffer[pos >> 5]);

			manBuffer[pos >> 5] = 0xFFFFFFFF;

			pos += 32;
		}

		if (siz > 0) {
			space -= (cnz(manBuffer[pos >> 5] >> (32 - siz)) + siz - 32);

			manBuffer[pos >> 5] |= (0xFFFFFFFF << (32 - siz));
		}
	}

	bool sset, eset;

	for (uint8_t inv = 1; inv <= BUFFER_DEPTH; ++inv) {
		sset = (bool)mget(spos + ((-((spos & 0x01) ^ 0x01)) | 0x01));
		eset = (bool)mget(epos + ((-((epos & 0x01) ^ 0x01)) | 0x01));

		spos = (spos - ((spos & 0x01) ^ 0x01)) >> 1;
		epos = (spos - ((spos & 0x01) ^ 0x01)) >> 1;

		pos = spos;
		siz = (epos - spos) + 1;

		if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
			manBuffer[pos >> 5] |= ((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
		}
		else {
			if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
				manBuffer[pos >> 5] |= (0xFFFFFFFF >> (pos & 0x1F));

				pos += (32 - (pos & 0x1F));
			}

			if (siz > 32) {
				memset(manBuffer + pos, 0xFF, ((epos - pos + 1) >> 5) << 2);

				siz &= 0x0000001F;
			}

			if (siz > 0) {
				manBuffer[pos >> 5] |= (0xFFFFFFFF << (32 - siz));
			}
		}

		mset(spos, sset);
		mset(epos, eset);
	}
}

void msez(uint32_t pos, uint32_t siz) {
	uint32_t spos = pos;
	uint32_t epos = (pos + siz - 1);

	if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
		space += (32 - cnz((manBuffer[pos >> 5] << (pos & 0x1F)) >> (32 - siz)));

		manBuffer[pos >> 5] &= ~((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
	}
	else {
		if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
			space += (32 - cnz(manBuffer[pos >> 5] << (pos & 0x1F)));

			manBuffer[pos >> 5] &= (0xFFFFFFFF << (32 - (pos & 0x1F)));

			pos += (32 - (pos & 0x1F));
		}

		for (; siz >= 32; siz -= 32) {
			space += (32 - cnz(manBuffer[pos >> 5]));

			manBuffer[pos >> 5] = 0x00000000;

			pos += 32;
		}

		if (siz > 0) {
			space += (32 - cnz(manBuffer[pos >> 5] >> (32 - siz)));

			manBuffer[pos >> 5] &= (0xFFFFFFFF >> siz);
		}
	}

	for (uint8_t inv = 1; inv <= BUFFER_DEPTH; ++inv) {
		spos = (spos - ((spos & 0x01) ^ 0x01)) >> 1;
		epos = (epos - ((epos & 0x01) ^ 0x01)) >> 1;

		pos = spos;
		siz = (epos - spos) + 1;

		if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
			manBuffer[pos >> 5] &= ~((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
		}
		else {
			if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
				manBuffer[pos >> 5] &= (0xFFFFFFFF << (32 - (pos & 0x1F)));

				pos += (32 - (pos & 0x1F));
			}

			if (siz > 32) {
				memset(manBuffer + pos, 0x00, ((epos - pos + 1) >> 5) << 2);

				siz &= 0x0000001F;
			}

			if (siz > 0) {
				manBuffer[epos >> 5] &= (0xFFFFFFFF >> siz);
			}
		}
	}
}

void mals(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec, uint32_t srt) {
	uint8_t level;
	
	if (srt) {
		for (level = BUFFER_DEPTH; level > 0; --level) {
			if ((srt & 0x01) && !mget(srt)) {
				srt += 1;

				break;
			}

			srt = (srt - ((srt & 0x01) ^ 0x01)) >> 1;
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

	uint32_t free = mgec(srt - 1, siz);

	vec.push_back(std::make_pair(srt, free));

	if (free != siz) return;
	
	mals(siz - free, vec, srt + free);
}

void mdis() {
	uint32_t pl = bpow - 1;
	uint32_t il = (bpow << 1) - 1;
	uint32_t sz = 0x01;
	uint32_t id = 0x00;

	for (uint8_t d = 0; d <= BUFFER_DEPTH; ++d) {
		std::cout << std::string(pl, ' ');

		for (uint8_t i = 0; i < sz; ++i) {
			if (i) std::cout << std::string(il, ' ');

			std::cout << bool(mget(id + i));
		}

		std::cout << std::string(pl, ' ') << std::endl;

		id += sz;
		sz <<= 1;

		il = pl;
		pl >>= 1;
	}
}

namespace manBuf {
	uint32_t spa() {
		return space;
	}

	bool get(uint32_t pos) {
		scheck(pos);

		return (bool)mget((pos | bpow) - 1);
	}

	bool get(uint32_t pos, uint32_t siz) {
		zcheck(siz);
		ocheck(siz, return get(pos););

		return (mgec((pos | bpow) - 1, siz) != siz);
	}

	void set(uint32_t pos, bool tog) {
		scheck(pos);

		mses((pos | bpow) - 1, tog);
	}

	void set(uint32_t pos, uint32_t siz, bool tog) {
		scheck(pos);
		scheck(pos + siz - 1);
		zcheck(siz);
		ocheck(siz, set(pos, tog); return;);

		if (tog) {
			mseo((pos | bpow) - 1, siz);
		}
		else {
			msez((pos | bpow) - 1, siz);
		}
	}

	void alo(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec) {
		if (mget(0)) return;

		zcheck(siz);

		mals(siz, vec, 0x00);
	}

	void dis() {
		ocheck(1, mdis(););
	}
}