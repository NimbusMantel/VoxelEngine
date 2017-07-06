#pragma once

#include <cstdint>
#include <vector>
#include <utility>
#include <memory>

#include "kernel/instruct.hpp"

#define BUFFER_DEPTH 23

#define VOXEL_DEPTH 16

#define BUFFER_SANITY 1

namespace manBuf {
	uint32_t spa();

	bool get(uint32_t pos);

	bool get(uint32_t pos, uint32_t siz);

	void set(uint32_t pos, bool tog);

	void set(uint32_t pos, uint32_t siz, bool tog);

	void alo(uint32_t siz, std::vector<std::pair<uint32_t, uint32_t>>& vec);

	void dis();
}

namespace manCTG {
	uint8_t* buf();

	void eqS(std::unique_ptr<INS_CTG> ins, bool fBg = false);

	void eqA(std::unique_ptr<INS_CTG> ins);

	uint32_t wri(uint32_t& syn, uint32_t& asy);
}

namespace manVox {
	void init();
}