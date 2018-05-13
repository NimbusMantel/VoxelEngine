#include "heap.hpp"

#include <intrin.h>
#include <utility>

#define serialiseUint24(memory, data) memory[0] = (memory[0] & 0x80) | ((data >> 16) & 0x7F); memory[1] = data >> 8; memory[2] = data;
#define deserialiseUint24(memory) (((memory[0] & 0x7F) << 16) | (memory[1] << 8) | memory[2])
#define copyUint24(dst, src) dst[0] = (dst[0] & 0x80) | (src[0] & 0x7F); dst[1] = src[1]; dst[2] = src[2];
#define markUint24(mark) mark |= 0x80;
#define unmarkUint24(mark) mark &= 0x7F;
#define flipMarkUint24(mark) mark ^= 0x80;
#define getMarkUint24(mark) (mark & 0x80)

const uint24_t mask = 0x049249;

struct SiblingGroup {
	uint8_t prev[3];

	union {
		uint8_t next[3];
		uint8_t mark;
	};
};

SiblingGroup* const dequeue = new SiblingGroup[voxels::size];

std::pair<uint24_t, uint24_t> pivot(uint24_t(voxels::size - 1), 0U);

uint8_t* const visibility = new uint8_t[voxels::size];

namespace voxels {
	uint8_t* init() {
		uint24_t prev = voxels::size - 1;
		uint24_t curr = 0;
		uint24_t next = 1;

		serialiseUint24(dequeue[0].prev, prev);
		serialiseUint24(dequeue[0].next, next);
		markUint24(dequeue[0].mark);

		for (uint32_t i = 1; i < (voxels::size - 1); i++) {
			prev = curr;
			curr = next;
			next = (_pext_u32(i + 1, mask << 2) << 14) | (_pext_u32(i + 1, mask << 1) << 7) | _pext_u32(i + 1, mask);

			serialiseUint24(dequeue[curr].prev, prev);
			serialiseUint24(dequeue[curr].next, next);
		}

		prev = curr;
		curr = voxels::size - 1;
		next = 0;

		serialiseUint24(dequeue[curr].prev, prev);
		serialiseUint24(dequeue[curr].next, next);

		return visibility;
	}

	uint32_t allocate() {
		const uint24_t next = deserialiseUint24(dequeue[0].next);

		// TO DO: deallocate if necessary
		
		copyUint24(dequeue[0].next, dequeue[next].next);
		const uint24_t dnext = deserialiseUint24(dequeue[next].next);
		serialiseUint24(dequeue[dnext].prev, 0);

		serialiseUint24(dequeue[next].prev, pivot.first);
		serialiseUint24(dequeue[next].next, pivot.second);
		markUint24(dequeue[next].mark);

		serialiseUint24(dequeue[pivot.first].next, next);
		serialiseUint24(dequeue[pivot.second].prev, next);

		pivot.second = next;

		return (next << 3);
	}

	void process() {
		bool prev;
		uint8_t curr;

		for (uint24_t i = 0; i < voxels::size; i++) {
			prev = getMarkUint24(dequeue[i].mark);
			curr = visibility[i];

			if (prev != bool(curr)) {
				const uint24_t qprev = deserialiseUint24(dequeue[i].prev);
				const uint24_t qnext = deserialiseUint24(dequeue[i].next);

				serialiseUint24(dequeue[qprev].next, qnext);
				serialiseUint24(dequeue[qnext].prev, qprev);

				serialiseUint24(dequeue[i].prev, pivot.first);
				serialiseUint24(dequeue[i].next, pivot.second);
				flipMarkUint24(dequeue[i].mark);

				serialiseUint24(dequeue[pivot.first].next, i);
				serialiseUint24(dequeue[pivot.second].prev, i);

				if (prev) {
					pivot.first = i;
				}
				else {
					pivot.second = i;
				}
			}
		}
	}
}