#pragma once

#include <cstdint>
#include <math.h>
#include <memory>

/*
Colour model according to "A physically Based Colour Model"
                       by Robert J Oddy and Philip J Willis
*/

/*KERNEL_BETSTOPS_BEG*/const uint8_t bets[9] = { 8, 16, 48, 96, 160, 208, 240, 248, 0 };/*KERNEL_BETSTOPS_END*/

struct col {
	col(float R, float G, float B) : r(R), g(G), b(B) {}
	col() : col(0.0f, 0.0f, 0.0f) {}
	
	float r;
	float g;
	float b;
};

struct mat {
	mat(col Par, float Bet, col Med) : par(Par), bet(Bet), med(Med) {}
	mat() : mat(col(), 0.0f, col(1.0f, 1.0f, 1.0f)) {}

	col par;
	float bet;
	col med;

	mat operator^(const mat& u) const {
		return mat(col(bet * par.r + (1.0f - bet) * med.r * u.bet * u.par.r, bet * par.g + (1.0f - bet) * med.g * u.bet * u.par.g, bet * par.b + (1.0f - bet) * med.b * u.bet * u.par.b),
				   bet + u.bet - bet * u.bet,
				   col(med.r * u.med.r, med.g * u.med.g, med.b * u.med.b)
		);
	}

	static mat abstract(mat (&mats)[8]) {
		mat sum = mat(col(0.0f, 0.0f, 0.0f), 0.0f, col(0.0f, 0.0f, 0.0f));
		uint8_t ems = 0;
		uint8_t acs = 0;

		mat tm1, tm2, tm3;

		for (uint8_t i = 0; i < 8; ++i) {
			tm1 = mats[i] ^ mats[i ^ 0x01];
			tm2 = mats[i] ^ mats[i ^ 0x02];
			tm3 = mats[i] ^ mats[i ^ 0x04];

			sum.par.r += (tm1.par.r + tm2.par.r + tm3.par.r);
			sum.par.g += (tm1.par.g + tm2.par.g + tm3.par.g);
			sum.par.b += (tm1.par.b + tm2.par.b + tm3.par.b);

			sum.bet += (tm1.bet + tm2.bet + tm3.bet);

			sum.med.r += (tm1.med.r + tm2.med.r + tm3.med.r);
			sum.med.g += (tm1.med.g + tm2.med.g + tm3.med.g);
			sum.med.b += (tm1.med.b + tm2.med.b + tm3.med.b);
		}

		const float fac = 1.0f / 24.0f;

		sum.par.r *= fac;
		sum.par.g *= fac;
		sum.par.b *= fac;

		sum.bet *= fac;

		sum.med.r *= fac;
		sum.med.g *= fac;
		sum.med.b *= fac;

		return sum;
	}

	uint32_t toBinary() {
		par.r = min(max(par.r, 0.0f), 1.0f);
		par.g = min(max(par.g, 0.0f), 1.0f);
		par.b = min(max(par.b, 0.0f), 1.0f);

		med.r = min(max(med.r, 0.0f), 1.0f);
		med.g = min(max(med.g, 0.0f), 1.0f);
		med.b = min(max(med.b, 0.0f), 1.0f);

		uint8_t beta = (uint8_t)roundf(bet * 255.0f);

		uint8_t parBits = 0x00;

		parBits |= (beta >= bets[parBits | 0x04]) << 2;
		parBits |= (beta >= bets[parBits | 0x02]) << 1;
		parBits |= (beta >= bets[parBits | 0x01]);
		parBits += (beta >= bets[parBits]);

		return ((((uint32_t)roundf(powf((par.r * beta) * (255.0f / (bets[parBits] - 1)), parBits / 8.0f))) << (32 - parBits)) |
			    (((uint32_t)roundf(powf((par.g * beta) * (255.0f / (bets[parBits] - 1)), parBits / 8.0f))) << (24 - parBits)) |
			    (((uint32_t)roundf(powf((par.b * beta) * (255.0f / (bets[parBits] - 1)), parBits / 8.0f))) << (16 - parBits)) |
			    (((uint32_t)roundf(powf((med.r * beta) * (255.0f / (bets[8 - parBits] - 1)), 1.0f - (parBits / 8.0f)))) << 24) |
			    (((uint32_t)roundf(powf((med.g * beta) * (255.0f / (bets[8 - parBits] - 1)), 1.0f - (parBits / 8.0f)))) << 16) |
			    (((uint32_t)roundf(powf((med.b * beta) * (255.0f / (bets[8 - parBits] - 1)), 1.0f - (parBits / 8.0f)))) << 8) |
			    beta
		);
	}
};

struct lit {
	virtual uint32_t toBinary() { return 0x00; }
};

struct mLit : public lit {
	mLit(col ClU, float BrU) : clU(ClU), brU(BrU) {}
	mLit() : mLit(col(), 0.0f) {}
	
	col clU;
	float brU;

	virtual uint32_t toBinary() override {
		float m = fmaxf(clU.r, fmaxf(clU.g, clU.b));
		if (m == 0.0f) m = 1.0f;

		clU.r = fmaxf(0.0f, clU.r / m);
		clU.g = fmaxf(0.0f, clU.g / m);
		clU.b = fmaxf(0.0f, clU.b / m);

		brU = fmaxf(0.0f, brU);

		uint32_t t = *((uint32_t*)(&brU));

		t += ((t & 0x00004000) << 1);
		if ((t & 0x7F800000) <= 0x39800000) t = 0x39800000;
		if ((t & 0x7F800000) > 0x49000000) t = 0x497F8000;

		return (0x00000000 | (((uint32_t)roundf(clU.r * 63.0f)) << 25) | (((uint32_t)roundf(clU.g * 63.0f)) << 19) | (((uint32_t)roundf(clU.b * 63.0f)) << 13) |
				(((t & 0x7FFF8000) - 0x38000000 - 0x01800000) >> 15)
		);
	}
};

struct bLit : public lit {
	bLit(col ClA, float BrA, col ClB, float BrB, uint8_t Dir) : clA(ClA), brA(BrA), clB(ClB), brB(BrB), dir(Dir) {}
	bLit() : bLit(col(), 0.0f, col(), 0.0f, 0x00) {}

	col clA, clB;
	float brA, brB;
	uint8_t dir;

	virtual uint32_t toBinary() override {
		float m = fmaxf(clA.r, fmaxf(clA.g, clA.b));
		if (m == 0.0f) m = 1.0f;

		clA.r = fmaxf(0.0f, clA.r / m);
		clA.g = fmaxf(0.0f, clA.g / m);
		clA.b = fmaxf(0.0f, clA.b / m);

		brA = fmaxf(0.0f, brA);

		m = fmaxf(clB.r, fmaxf(clB.g, clB.b));
		if (m == 0.0f) m = 1.0f;

		clB.r = fmaxf(0.0f, clB.r / m);
		clB.g = fmaxf(0.0f, clB.g / m);
		clB.b = fmaxf(0.0f, clB.b / m);

		brB = fmaxf(0.0f, brB);

		uint32_t tA = *((uint32_t*)(&brA));
		uint32_t tB = *((uint32_t*)(&brB));

		tA += ((tA & 0x00400000) << 1);
		if ((tA & 0x7F800000) <= 0x39800000) tA = 0x39800000;
		if ((tA & 0x7F800000) > 0x49000000) tA = 0x497F8000;

		tB += ((tB & 0x00400000) << 1);
		if ((tB & 0x7F800000) <= 0x39800000) tB = 0x39800000;
		if ((tB & 0x7F800000) > 0x49000000) tB = 0x497F8000;

		return (0x80000000 | (((uint32_t)(dir & 0x07)) << 28) | (((uint32_t)roundf(clA.r * 7.0f)) << 25) | (((uint32_t)roundf(clA.g * 7.0f)) << 22) | (((uint32_t)roundf(clA.b * 7.0f)) << 19) |
				(((tA & 0x7F800000) - 0x38000000 - 0x01800000) >> 9) |
				(((uint32_t)roundf(clB.r * 7.0f)) << 11) | (((uint32_t)roundf(clB.g * 7.0f)) << 8) | (((uint32_t)roundf(clB.b * 7.0f)) << 5) |
				(((tB & 0x7F800000) - 0x38000000 - 0x01800000) >> 23)
		);
	}
};

struct vis {
	vis(mat Mat, lit* Lit) : mal(Mat), lig(std::move(Lit)) {}
	vis() : vis(mat(), new mLit()) {}
	
	mat mal;
	std::unique_ptr<lit> lig;

	uint64_t toBinary() {
		return ((((uint64_t)mal.toBinary()) << 32) | lig->toBinary());
	}
};