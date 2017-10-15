#pragma once

#include <cstdint>
#include <vector>
#include <utility>
#include <memory>

#include "kernel/instruct.hpp"

#define BUFFER_DEPTH 20

#define VOXEL_DEPTH 16

#define UNIT_DEPTH 4

#define BUFFER_SANITY 1

namespace manBuf {
	uint32_t spa();

	float per();

	bool get(uint32_t pos);

	bool get(uint32_t pos, uint32_t siz);

	void set(uint32_t pos, bool tog);

	void set(uint32_t pos, bool tog, uint32_t siz);

	void alo(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec);

	void dis();
}

namespace manCTG {
	uint8_t* buf();

	void eqS(std::unique_ptr<INS_CTG> ins, bool fBg = false);

	void eqA(std::unique_ptr<INS_CTG> ins);

	uint32_t wri(uint32_t& syn, uint32_t& asy);
}

namespace manGTC {
	uint8_t* buf();
}

namespace manVox {
	void init();
}