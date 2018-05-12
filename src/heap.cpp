#include "heap.hpp"

#include <intrin.h>
#include <utility>

#include <iostream>

const uint32_t mask = 0x00049249;

uint32_t* const dequeue = new uint32_t[voxels::size << 1];

std::pair<uint32_t, uint32_t> pivot(uint32_t(voxels::size - 1), 0U);

uint8_t* const visibility = new uint8_t[voxels::size];

namespace voxels {
	uint8_t* init() {
		uint32_t prev = voxels::size - 1;
		uint32_t curr = 0;
		uint32_t next = 1;

		dequeue[0] = prev << 11;
		dequeue[1] = 0x80000000 | next;

		for (uint32_t i = 1; i < (voxels::size - 1); i++) {
			prev = curr;
			curr = next;
			next = (_pext_u32(i + 1, mask << 2) << 14) | (_pext_u32(i + 1, mask << 1) << 7) | _pext_u32(i + 1, mask);

			dequeue[(curr << 1) | 0x00] = prev << 11;
			dequeue[(curr << 1) | 0x01] = next;
		}

		prev = curr;
		curr = voxels::size - 1;
		next = 0;

		dequeue[(curr << 1) | 0x00] = prev << 11;
		dequeue[(curr << 1) | 0x01] = next;

		return visibility;
	}

	uint32_t allocate(uint32_t parent) {
		const uint32_t next = dequeue[1] & 0x001FFFFF;

		// TO DO: deallocate if necessary

		dequeue[1] = 0x80000000 | (dequeue[(next << 1) | 0x01] & 0x001FFFFF);
		dequeue[(dequeue[(next << 1) | 0x01] & 0x001FFFFF) << 1] &= 0x000007FF;

		parent >>= 3;

		dequeue[(next << 1) | 0x00] = (pivot.first << 11) | (parent >> 10);
		dequeue[(next << 1) | 0x01] = 0x80000000 | (parent << 21) | pivot.second;

		dequeue[(pivot.first << 1) | 0x01] = (dequeue[(pivot.first << 1) | 0x01] & 0xFFE00000) | next;
		dequeue[(pivot.second << 1) | 0x0] = (next << 11) | (dequeue[(pivot.second << 1) | 0x0] & 0x000007FF);

		pivot.second = next;

		return (next << 3);
	}

	void process() {
		// TO DO

		/*uint8_t* begin = visibility;
		uint8_t* pointer = begin;
		uint8_t* end = begin + size;

		std::cout << "Visible voxels: ";

		while (pointer < end) {
			if (*pointer != 0x00) {
				printf("%u(0x%02X) ", uint32_t(pointer - begin), *pointer);
			}

			pointer++;
		}

		std::cout << std::endl;*/
	}
}