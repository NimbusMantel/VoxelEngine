#include "voxel.hpp"

#include <assert.h>
#include <intrin.h>
#include <iostream>
#include <bitset>
#include <queue>

#define ceil(n) ((int)(n) + (n > (int)(n)))

static uint32_t __inline clz(uint32_t n) {
	unsigned long r; bool z = _BitScanReverse(&r, n); return (32 - z - r);
}

static uint32_t __inline ctz(uint32_t n) {
	unsigned long r; bool z = _BitScanForward(&r, n); return (r | (32 & (~(-z))));
}

static uint32_t __inline cnz(uint32_t n) {
	return (32 - __popcnt(n));
}

static const uint32_t mpow = (0x01 << BUFFER_DEPTH);

static uint32_t manBuffer[ceil((0xFFFFFFFF >> (31 - BUFFER_DEPTH)) / 32.0f)];

static uint32_t mspace = mpow;

#if BUFFER_SANITY
#define scheck(p) assert(p < mpow);
#define zcheck(p) assert(p != 0);
#define ocheck(p, a) if (p == 1) {a}
#else
#define scheck(p) 
#define zcheck(p) 
#define ockeck(p, a) 
#endif

#define mget(p) (manBuffer[(p) >> 5] & (0x80000000 >> ((p) & 0x1F)))
#define mset(p, b) manBuffer[(p) >> 5] = (manBuffer[(p) >> 5] & (0xFFFFFFFF ^ (0x80000000 >> ((p) & 0x1F)))) | (((uint32_t)(b)) << (31 - ((p) & 0x1F)))

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

	mspace += ((-tog) | 0x01);

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
		mspace -= (cnz((manBuffer[pos >> 5] << (pos & 0x1F)) >> (32 - siz)) + siz - 32);

		manBuffer[pos >> 5] |= ((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
	}
	else {
		if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
			mspace -= (cnz(manBuffer[pos >> 5] << (pos & 0x1F)) - (pos & 0x1F));

			manBuffer[pos >> 5] |= (0xFFFFFFFF >> (pos & 0x1F));

			siz -= (32 - (pos & 0x1F));
			pos += (32 - (pos & 0x1F));
		}

		for (; siz >= 32; siz -= 32) {
			mspace -= cnz(manBuffer[pos >> 5]);

			manBuffer[pos >> 5] = 0xFFFFFFFF;

			pos += 32;
		}

		if (siz > 0) {
			mspace -= (cnz(manBuffer[pos >> 5] >> (32 - siz)) + siz - 32);

			manBuffer[pos >> 5] |= (0xFFFFFFFF << (32 - siz));
		}
	}
	
	bool sset = true;
	bool eset = true;

	for (uint8_t inv = 1; inv <= BUFFER_DEPTH; ++inv) {
		sset = sset && (bool)mget(spos + ((-((spos & 0x01) ^ 0x01)) | 0x01));
		eset = eset && (bool)mget(epos + ((-((epos & 0x01) ^ 0x01)) | 0x01));

		spos = (spos - ((spos & 0x01) ^ 0x01)) >> 1;
		epos = (epos - ((epos & 0x01) ^ 0x01)) >> 1;
		
		pos = spos;
		siz = (epos - spos) + 1;
		
		if ((pos & 0x1F) && siz <= (32 - (pos & 0x1F))) {
			manBuffer[pos >> 5] |= ((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
		}
		else {
			if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
				manBuffer[pos >> 5] |= (0xFFFFFFFF >> (pos & 0x1F));

				siz -= (32 - (pos & 0x1F));
				pos += (32 - (pos & 0x1F));
			}

			if (siz > 32) {
				memset(manBuffer + pos, 0xFF, ((epos - pos + 1) >> 5) << 2);

				siz &= 0x0000001F;
			}

			if (siz > 0) {
				manBuffer[epos >> 5] |= (0xFFFFFFFF << (32 - siz));
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
		mspace += (32 - cnz((manBuffer[pos >> 5] << (pos & 0x1F)) >> (32 - siz)));

		manBuffer[pos >> 5] &= ~((0xFFFFFFFF >> (32 - siz)) << (32 - siz - (pos & 0x1F)));
	}
	else {
		if ((pos & 0x1F) && siz > (32 - (pos & 0x1F))) {
			mspace += (32 - cnz(manBuffer[pos >> 5] << (pos & 0x1F)));

			manBuffer[pos >> 5] &= (0xFFFFFFFF << (32 - (pos & 0x1F)));

			siz -= (32 - (pos & 0x1F));
			pos += (32 - (pos & 0x1F));
		}

		for (; siz >= 32; siz -= 32) {
			mspace += (32 - cnz(manBuffer[pos >> 5]));

			manBuffer[pos >> 5] = 0x00000000;

			pos += 32;
		}

		if (siz > 0) {
			mspace += (32 - cnz(manBuffer[pos >> 5] >> (32 - siz)));

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
				
				siz -= (32 - (pos & 0x1F));
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
			if ((srt & 0x01) && !mget(srt + 1)) {
				srt += 1;

				break;
			}

			srt = (srt - ((srt & 0x01) ^ 0x01)) >> 1;
		}

		if (level == 0) return;
	}
	else {
		level = 1;

		srt = 1 + (bool)mget(0x01);
	}
	
	for (; level < BUFFER_DEPTH; ++level) {
		srt = ((srt + 1) << 1);
		srt -= ((bool)mget(srt - 1) ^ 0x01);
	}

	uint32_t free = mgec(srt, siz);

	vec.push_back(std::make_pair((srt + 1) & (~mpow), siz ^ ((free ^ siz) & -(free < siz))));

	if (free < siz) {
		mals(siz - free, vec, srt + free);
	}
}

void mdis() {
	uint32_t pl = mpow - 1;
	uint32_t il = (mpow << 1) - 1;
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
		return mspace;
	}

	bool get(uint32_t pos) {
		scheck(pos);

		return (bool)mget((pos | mpow) - 1);
	}

	bool get(uint32_t pos, uint32_t siz) {
		zcheck(siz);
		ocheck(siz, return get(pos););

		return (mgec((pos | mpow) - 1, siz) != siz);
	}

	void set(uint32_t pos, bool tog) {
		scheck(pos);

		mses((pos | mpow) - 1, tog);
	}

	void set(uint32_t pos, uint32_t siz, bool tog) {
		scheck(pos);
		scheck(pos + siz - 1);
		zcheck(siz);
		ocheck(siz, set(pos, tog); return;);

		if (tog) {
			mseo((pos | mpow) - 1, siz);
		}
		else {
			msez((pos | mpow) - 1, siz);
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

static const uint8_t cPriorityMax = (0x01 << 4) - 1;

static uint8_t cPriorityIndex = 0;

std::queue<std::unique_ptr<INS_CTG>> cPriorityQueue[cPriorityMax + 1] = {std::queue<std::unique_ptr<INS_CTG>>()};
std::vector<std::unique_ptr<INS_CTG>> cAsyncInstructs[INS_CTG::NUM] = {std::vector<std::unique_ptr<INS_CTG>>()};
std::vector<std::unique_ptr<INS_CTG>> cSyncInstructs = {};

namespace manCtG {
	void eqS(std::unique_ptr<INS_CTG> ins) {
		uint8_t pi = (cPriorityIndex + ins->PRI()) & cPriorityMax;

		ins->syn = (cPriorityQueue[pi].empty() ? false : (cPriorityQueue[pi].back()->OPC() == ins->OPC()));

		cPriorityQueue[pi].push(std::move(ins));
	}

	void eqA(std::unique_ptr<INS_CTG> ins) {
		uint8_t pi = (cPriorityIndex + ins->PRI()) & cPriorityMax;

		ins->syn = false;

		cPriorityQueue[pi].push(std::move(ins));
	}

	uint32_t wri(uint8_t* buf, uint32_t& syn, uint32_t& asy) {
		uint32_t size = 0;
		syn = 0;
		asy = 0;

		// TO DO: Check with the max size and stop accordingly

		std::unique_ptr<INS_CTG> ptr;

		for (uint8_t pi = (cPriorityIndex - 1) & cPriorityMax; pi != cPriorityIndex; pi = (pi - 1) & cPriorityMax) {
			while (cPriorityQueue[pi].size() > 0) {
				ptr = std::move(cPriorityQueue[pi].front());

				cPriorityQueue[pi].pop();

				if (ptr->syn) {
					cSyncInstructs.push_back(std::move(ptr));
				}
				else if (cPriorityQueue[pi].size() > 0 && cPriorityQueue[pi].front()->syn) {
					cSyncInstructs.push_back(std::move(ptr));

					syn += 1;
				}
				else {
					cAsyncInstructs[ptr->OPC()].push_back(std::move(ptr));

					asy += 1;
				}
			}
		}

		uint32_t bIdx = -7;
		uint32_t headers = 0;

		for (uint32_t i = 0; i < cSyncInstructs.size(); ++i) {
			if (!cSyncInstructs[i]->syn) {
				bIdx += 7;
				headers += 1;

				buf[bIdx] = cSyncInstructs[i]->OPC();
				buf[bIdx + 1] = buf[bIdx + 2] = 0x00;
				buf[bIdx + 3] = 0x01;
			}
			else {
				buf[bIdx + 1] += 1;
				buf[bIdx + 2] += (bool)buf[bIdx + 1];
				buf[bIdx + 3] += ((bool)buf[bIdx + 1] && (bool)buf[bIdx + 2]);
			}
		}

		for (uint8_t i = 0; i < INS_CTG::NUM; ++i) {
			if (cAsyncInstructs[INS_CTG::ALL[i]].size()) {
				bIdx += 7;
				headers += 1;

				buf[bIdx] = INS_CTG::ALL[i];
				buf[bIdx + 1] = cAsyncInstructs[INS_CTG::ALL[i]].size() & 0xFF0000;
				buf[bIdx + 2] = cAsyncInstructs[INS_CTG::ALL[i]].size() & 0x00FF00;
				buf[bIdx + 3] = cAsyncInstructs[INS_CTG::ALL[i]].size() & 0x0000FF;
			}
		}

		size = bIdx + 7;

		uint32_t amount;
		uint32_t iIdx = 0;

		uint8_t insCode;
		uint32_t insSize;

		std::vector<std::unique_ptr<INS_CTG>>* insData;

		for (bIdx = 0; bIdx < headers; ++bIdx) {
			amount = ((((uint32_t)buf[bIdx * 7 + 1]) << 16) | (((uint16_t)buf[bIdx * 7 + 2]) << 8) | buf[bIdx * 7 + 3]);

			buf[bIdx + 4] = size & 0x0000FF;
			buf[bIdx + 5] = size & 0x00FF00;
			buf[bIdx + 6] = size & 0xFF0000;

			if (headers < syn) {
				insCode = buf[bIdx];
				insSize = cSyncInstructs[iIdx]->SIZ();

				insData = &cSyncInstructs;
			}
			else {
				insCode = buf[bIdx];
				insSize = cAsyncInstructs[insCode][0]->SIZ();

				insData = &cAsyncInstructs[insCode];
			}

			for (uint32_t i = 0; i < amount; ++i) {
				(*insData)[iIdx]->WRI(buf + size);

				iIdx += 1;

				size += insSize;
			}
		}

		cPriorityIndex = (cPriorityIndex + 1) & cPriorityMax;

		cSyncInstructs.clear();

		for (uint8_t i = 0; i < INS_CTG::NUM; ++i) {
			cAsyncInstructs[i].clear();
		}

		return size;
	}
}