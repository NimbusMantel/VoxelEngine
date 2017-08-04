#pragma once

#include <cstdint>

/*
Colour model according to "A physically Based Colour Model"
                       by Robert J Oddy and Philip J Willis
*/

struct col {
	col(float R, float G, float B) : r(R), g(G), b(B) {}
	col() : col(0.0f, 0.0f, 0.0f) {}
	
	float r;
	float g;
	float b;
};

struct mat {
	mat(col Par, float Bet, col Med, bool Emi) : par(Par), bet(Bet), med(Med), emi(Emi), act(true) {}
	mat() : mat(col(), 0.0f, col(1.0f, 1.0f, 1.0f), false) { act = false; }

	bool act;

	col par;
	float bet;
	col med;
	bool emi;

	mat operator^(const mat& u) const {
		return mat(col(bet * par.r + (1.0f - bet) * med.r * u.bet * u.par.r, bet * par.g + (1.0f - bet) * med.g * u.bet * u.par.g, bet * par.b + (1.0f - bet) * med.b * u.bet * u.par.b),
				   bet + u.bet - bet * u.bet,
				   col(med.r * u.med.r, med.g * u.med.g, med.b * u.med.b),
				   false
		);
	}

	static mat abstract(mat (&mats)[8]) {
		mat sum = mat(col(0.0f, 0.0f, 0.0f), 0.0f, col(0.0f, 0.0f, 0.0f), false);
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

			ems += mats[i].emi;
			acs += mats[i].act;
		}

		const float fac = 1.0f / 24.0f;

		sum.par.r *= fac;
		sum.par.g *= fac;
		sum.par.b *= fac;

		sum.bet *= fac;

		sum.med.r *= fac;
		sum.med.g *= fac;
		sum.med.b *= fac;

		sum.emi = (ems >= ((acs + 1) >> 1));

		return sum;
	}

	uint64_t toBinary() const {
		return ((((uint64_t)roundf(par.r * 63.0f)) << 58) | (((uint64_t)roundf(par.g * 63.0f)) << 52) | (((uint64_t)roundf(par.b * 63.0f)) << 46) |
				(((uint64_t)roundf(bet * 63.0f)) << 40) |
				(((uint64_t)roundf(med.r * 31.0f)) << 35) | (((uint64_t)roundf(med.g * 31.0f)) << 30) | (((uint64_t)roundf(med.b * 31.0f)) << 25) |
				(((uint64_t)emi) << 24) |
				0x000000
		);
	}
};