#pragma once

#include <cstdint>

namespace Colour {
	const uint16_t sqrs[256] = { 0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225, 256, 289, 324, 361, 400, 441, 484, 529, 576, 625, 676, 729, 784, 841, 900, 961, 1024, 1089, 1156, 1225, 1296, 1369, 1444, 1521, 1600, 1681, 1764, 1849, 1936, 2025, 2116, 2209, 2304, 2401, 2500, 2601, 2704, 2809, 2916, 3025, 3136, 3249, 3364, 3481, 3600, 3721, 3844, 3969, 4096, 4225, 4356, 4489, 4624, 4761, 4900, 5041, 5184, 5329, 5476, 5625, 5776, 5929, 6084, 6241, 6400, 6561, 6724, 6889, 7056, 7225, 7396, 7569, 7744, 7921, 8100, 8281, 8464, 8649, 8836, 9025, 9216, 9409, 9604, 9801, 10000, 10201, 10404, 10609, 10816, 11025, 11236, 11449, 11664, 11881, 12100, 12321, 12544, 12769, 12996, 13225, 13456, 13689, 13924, 14161, 14400, 14641, 14884, 15129, 15376, 15625, 15876, 16129, 16384, 16641, 16900, 17161, 17424, 17689, 17956, 18225, 18496, 18769, 19044, 19321, 19600, 19881, 20164, 20449, 20736, 21025, 21316, 21609, 21904, 22201, 22500, 22801, 23104, 23409, 23716, 24025, 24336, 24649, 24964, 25281, 25600, 25921, 26244, 26569, 26896, 27225, 27556, 27889, 28224, 28561, 28900, 29241, 29584, 29929, 30276, 30625, 30976, 31329, 31684, 32041, 32400, 32761, 33124, 33489, 33856, 34225, 34596, 34969, 35344, 35721, 36100, 36481, 36864, 37249, 37636, 38025, 38416, 38809, 39204, 39601, 40000, 40401, 40804, 41209, 41616, 42025, 42436, 42849, 43264, 43681, 44100, 44521, 44944, 45369, 45796, 46225, 46656, 47089, 47524, 47961, 48400, 48841, 49284, 49729, 50176, 50625, 51076, 51529, 51984, 52441, 52900, 53361, 53824, 54289, 54756, 55225, 55696, 56169, 56644, 57121, 57600, 58081, 58564, 59049, 59536, 60025, 60516, 61009, 61504, 62001, 62500, 63001, 63504, 64009, 64516, 65025 };

	typedef uint64_t colPBM;
	typedef uint32_t colRGB;

	inline colPBM col_to_par(colPBM col) {
		return ((col & 0xFFFFFF0000000000) >> 40);
	}

	inline colPBM par_to_col(colPBM par) {
		return ((par & 0x0000000000FFFFFF) << 40);
	}

	inline colPBM col_to_bet(colPBM col) {
		return ((col & 0x000000FF00000000) >> 32);
	}

	inline colPBM bet_to_col(colPBM bet) {
		return ((bet & 0x00000000000000FF) << 32);
	}

	inline colPBM col_to_med(colPBM col) {
		return ((col & 0x00000000FFFFFF00) >> 8);
	}

	inline colPBM med_to_col(colPBM med) {
		return ((med & 0x0000000000FFFFFF) << 8);
	}

	inline colPBM rgb_to_red(colPBM rgb) {
		return ((rgb & 0xFF0000) >> 16);
	}

	inline colPBM rgb_to_gre(colPBM rgb) {
		return ((rgb & 0x00FF00) >> 8);
	}

	inline colPBM rgb_to_blu(colPBM rgb) {
		return (rgb & 0x0000FF);
	}

	inline colPBM rgb_to_rgb(colPBM red, colPBM gre, colPBM blu) {
		return ((red << 16) | (gre << 8) | blu);
	}

	inline colPBM col_sqre(colPBM col) {
		return (col * col);
	}

	inline colPBM col_sqrt(colPBM col) {
		colPBM s = 0x00;

		s |= (col >= sqrs[s | 0x80]) << 7;
		s |= (col >= sqrs[s | 0x40]) << 6;
		s |= (col >= sqrs[s | 0x20]) << 5;
		s |= (col >= sqrs[s | 0x10]) << 4;
		s |= (col >= sqrs[s | 0x08]) << 3;
		s |= (col >= sqrs[s | 0x04]) << 2;
		s |= (col >= sqrs[s | 0x02]) << 1;
		s |= (col >= sqrs[s | 0x01]);

		return s;
	}

	inline colPBM col_dv_min(colPBM col) {
		return (col + (col == 0x00));
	}

	inline colPBM col_pl_col(colPBM colA, colPBM colB) {
		return par_to_col(rgb_to_rgb(col_sqrt((col_to_bet(colA)*col_sqre(rgb_to_red(col_to_par(colA))) + col_to_bet(colB)*col_sqre(rgb_to_red(col_to_par(colB)))) / col_dv_min(col_to_bet(colA) + col_to_bet(colB))), col_sqrt((col_to_bet(colA)*col_sqre(rgb_to_gre(col_to_par(colA))) + col_to_bet(colB)*col_sqre(rgb_to_gre(col_to_par(colB)))) / col_dv_min(col_to_bet(colA) + col_to_bet(colB))), col_sqrt((col_to_bet(colA)*col_sqre(rgb_to_blu(col_to_par(colA))) + col_to_bet(colB)*col_sqre(rgb_to_blu(col_to_par(colB)))) / col_dv_min(col_to_bet(colA) + col_to_bet(colB))))) | bet_to_col((col_to_bet(colA) + col_to_bet(colB)) >> 1) | med_to_col(rgb_to_rgb(col_sqrt(((0xff - col_to_bet(colA))*col_sqre(rgb_to_red(col_to_med(colA))) + (0xff - col_to_bet(colB))*col_sqre(rgb_to_red(col_to_med(colB)))) / col_dv_min(0x01fe - col_to_bet(colA) - col_to_bet(colB))), col_sqrt(((0xff - col_to_bet(colA))*col_sqre(rgb_to_gre(col_to_med(colA))) + (0xff - col_to_bet(colB))*col_sqre(rgb_to_gre(col_to_med(colB)))) / col_dv_min(0x01fe - col_to_bet(colA) - col_to_bet(colB))), col_sqrt(((0xff - col_to_bet(colA))*col_sqre(rgb_to_blu(col_to_med(colA))) + (0xff - col_to_bet(colB))*col_sqre(rgb_to_blu(col_to_med(colB)))) / col_dv_min(0x01fe - col_to_bet(colA) - col_to_bet(colB)))));
	}

	inline colPBM col_ov_col(colPBM colA, colPBM colB) {
		return par_to_col(rgb_to_rgb((((col_to_bet(colA)*rgb_to_red(col_to_par(colA))) >> 8) + ((rgb_to_red(col_to_med(colA))*(0xff - col_to_bet(colA))*col_to_bet(colB)*rgb_to_red(col_to_par(colB))) >> 16)), (((col_to_bet(colA)*rgb_to_gre(col_to_par(colA))) >> 8) + ((rgb_to_gre(col_to_med(colA))*(0xff - col_to_bet(colA))*col_to_bet(colB)*rgb_to_gre(col_to_par(colB))) >> 16)), (((col_to_bet(colA)*rgb_to_blu(col_to_par(colA))) >> 8) + ((rgb_to_blu(col_to_med(colA))*(0xff - col_to_bet(colA))*col_to_bet(colB)*rgb_to_blu(col_to_par(colB))) >> 16)))) | bet_to_col(col_to_bet(colA) + (((0xff - col_to_bet(colA))*col_to_bet(colB)) >> 8)) | med_to_col(rgb_to_rgb(((rgb_to_red(col_to_med(colA))*rgb_to_red(col_to_med(colB))) >> 8), ((rgb_to_gre(col_to_med(colA))*rgb_to_gre(col_to_med(colB))) >> 8), ((rgb_to_blu(col_to_med(colA))*rgb_to_blu(col_to_med(colB))) >> 8)));
	}

	inline colRGB col_to_pix(colPBM col, colRGB fro, colRGB bac) {
		return (colRGB)rgb_to_rgb(col_sqrt(((col_to_bet(col)*col_sqre((rgb_to_red(col_to_par(col))*rgb_to_red(fro)) >> 8)) + ((0xff - col_to_bet(col))*col_sqre((rgb_to_red(col_to_med(col))*rgb_to_red(bac)) >> 8))) >> 8), col_sqrt(((col_to_bet(col)*col_sqre((rgb_to_gre(col_to_par(col))*rgb_to_gre(fro)) >> 8)) + ((0xff - col_to_bet(col))*col_sqre((rgb_to_gre(col_to_med(col))*rgb_to_gre(bac)) >> 8))) >> 8), col_sqrt(((col_to_bet(col)*col_sqre((rgb_to_blu(col_to_par(col))*rgb_to_blu(fro)) >> 8)) + ((0xff - col_to_bet(col))*col_sqre((rgb_to_blu(col_to_med(col))*rgb_to_blu(bac)) >> 8))) >> 8));
	}
}