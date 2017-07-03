#pragma once

#include <cstdint>

/*
Colour model according to "A physically Based Colour Model"
                       by Robert J Oddy and Philip J Willis
*/

namespace Material {
	const uint32_t cbrs[64] = { 0, 1, 8, 27, 64, 125, 216, 343, 512, 729, 1000, 1331, 1728, 2197, 2744, 3375, 4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167, 13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791, 32768, 35937, 39304, 42875, 46656, 50653, 54872, 59319, 64000, 68921, 74088, 79507, 85184, 91125, 97336, 103823, 110592, 117649, 125000, 132651, 140608, 148877, 157464, 166375, 175616, 185193, 195112, 205379, 216000, 226981, 238328, 250047 };

	typedef uint64_t matPbMl;

	inline matPbMl mat_to_parR(matPbMl mat) {
		return ((mat & 0xFC00000000000000) >> 58);
	}

	inline matPbMl parR_to_mat(matPbMl parR) {
		return ((parR & 0x000000000000003F) << 58);
	}

	inline matPbMl mat_to_parG(matPbMl mat) {
		return ((mat & 0x03F0000000000000) >> 52);
	}

	inline matPbMl parG_to_mat(matPbMl parG) {
		return ((parG & 0x000000000000003F) << 52);
	}

	inline matPbMl mat_to_parB(matPbMl mat) {
		return ((mat & 0x000FC00000000000) >> 46);
	}

	inline matPbMl parB_to_mat(matPbMl parB) {
		return ((parB & 0x000000000000003F) << 46);
	}

	inline matPbMl mat_to_beta(matPbMl mat) {
		return ((mat & 0x00003F0000000000) >> 40);
	}

	inline matPbMl beta_to_mat(matPbMl beta) {
		return ((beta & 0x000000000000003F) << 40);
	}

	inline matPbMl mat_to_medR(matPbMl mat) {
		return ((mat & 0x000000F800000000) >> 35);
	}

	inline matPbMl medR_to_mat(matPbMl medR) {
		return ((medR & 0x000000000000001F) << 35);
	}

	inline matPbMl mat_to_medG(matPbMl mat) {
		return ((mat & 0x00000007C0000000) >> 30);
	}

	inline matPbMl medG_to_mat(matPbMl medG) {
		return ((medG & 0x000000000000001F) << 30);
	}

	inline matPbMl mat_to_medB(matPbMl mat) {
		return ((mat & 0x000000003E000000) >> 25);
	}

	inline matPbMl medB_to_mat(matPbMl medB) {
		return ((medB & 0x000000000000001F) << 25);
	}

	inline matPbMl mat_to_emit(matPbMl mat) {
		return ((mat & 0x0000000001000000) >> 24);
	}

	inline matPbMl emit_to_mat(matPbMl emit) {
		return ((emit & 0x0000000000000001) << 24);
	}

	inline matPbMl mat_to_col(matPbMl mat) {
		return ((mat & 0xFFFFFFFFFF000000) >> 24);
	}

	inline matPbMl col_to_mat(matPbMl col) {
		return ((col & 0x000000FFFFFFFFFF) << 24);
	}

	inline matPbMl mat_to_litR(matPbMl mat) {
		return ((mat & 0x0000000000FC0000) >> 18);
	}

	inline matPbMl litR_to_mat(matPbMl litR) {
		return ((litR & 0x000000000000003F) << 18);
	}

	inline matPbMl mat_to_litG(matPbMl mat) {
		return ((mat & 0x000000000003F000) >> 12);
	}

	inline matPbMl litG_to_mat(matPbMl litG) {
		return ((litG & 0x000000000000003F) << 12);
	}

	inline matPbMl mat_to_litB(matPbMl mat) {
		return ((mat & 0x0000000000000FC0) >> 6);
	}

	inline matPbMl litB_to_mat(matPbMl litB) {
		return ((litB & 0x000000000000003F) << 6);
	}

	inline matPbMl mat_to_litH(matPbMl mat) {
		return (mat & 0x000000000000003F);
	}

	inline matPbMl litH_to_mat(matPbMl litH) {
		return (litH & 0x000000000000003F);
	}

	inline matPbMl mat_to_lit(matPbMl mat) {
		return (mat & 0x0000000000FFFFFF);
	}

	inline matPbMl lit_to_mat(matPbMl lit) {
		return (lit & 0x0000000000FFFFFF);
	}

	inline matPbMl mat_cube(matPbMl val) {
		return (val * val * val);
	}
	
	inline matPbMl mat_cbrt(matPbMl val) {
		matPbMl s = 0x00;

		s |= (val >= cbrs[s | 0x80]) << 7;
		s |= (val >= cbrs[s | 0x40]) << 6;
		s |= (val >= cbrs[s | 0x20]) << 5;
		s |= (val >= cbrs[s | 0x10]) << 4;
		s |= (val >= cbrs[s | 0x08]) << 3;
		s |= (val >= cbrs[s | 0x04]) << 2;
		s |= (val >= cbrs[s | 0x02]) << 1;
		s |= (val >= cbrs[s | 0x01]);

		return s;
	}

	inline matPbMl mat_dv_min(matPbMl val) {
		return (val | (val == 0x00));
	}

	inline matPbMl col_pl_col(matPbMl matA, matPbMl matB) {
		return (parR_to_mat(mat_cbrt((mat_to_beta(matA) * mat_cube(mat_to_parR(matA)) + mat_to_beta(matB) * mat_cube(mat_to_parR(matB))) / mat_dv_min(mat_to_beta(matA) + mat_to_beta(matB)))) |
			parG_to_mat(mat_cbrt((mat_to_beta(matA) * mat_cube(mat_to_parG(matA)) + mat_to_beta(matB) * mat_cube(mat_to_parG(matB))) / mat_dv_min(mat_to_beta(matA) + mat_to_beta(matB)))) |
			parB_to_mat(mat_cbrt((mat_to_beta(matA) * mat_cube(mat_to_parB(matA)) + mat_to_beta(matB) * mat_cube(mat_to_parB(matB))) / mat_dv_min(mat_to_beta(matA) + mat_to_beta(matB)))) |
			beta_to_mat(mat_cbrt((mat_cube(mat_to_beta(matA)) + mat_cube(mat_to_beta(matB))) >> 1)) |
			medR_to_mat(mat_cbrt(((63 - mat_to_beta(matA)) * mat_cube(mat_to_medR(matA)) + (63 - mat_to_beta(matB)) * mat_cube(mat_to_medR(matB))) / mat_dv_min(126 - mat_to_beta(matA) - mat_to_beta(matB)))) |
			medG_to_mat(mat_cbrt(((63 - mat_to_beta(matA)) * mat_cube(mat_to_medG(matA)) + (63 - mat_to_beta(matB)) * mat_cube(mat_to_medG(matB))) / mat_dv_min(126 - mat_to_beta(matA) - mat_to_beta(matB)))) |
			medB_to_mat(mat_cbrt(((63 - mat_to_beta(matA)) * mat_cube(mat_to_medB(matA)) + (63 - mat_to_beta(matB)) * mat_cube(mat_to_medB(matB))) / mat_dv_min(126 - mat_to_beta(matA) - mat_to_beta(matB)))) |
			emit_to_mat(mat_to_emit(matA) & mat_to_emit(matB)));
	}

	inline matPbMl mat_avg_col(matPbMl m0, matPbMl m1, matPbMl m2, matPbMl m3, matPbMl m4, matPbMl m5, matPbMl m6, matPbMl m7) {
		return (parR_to_mat(mat_cbrt((mat_to_beta(m0) * mat_cube(mat_to_parR(m0)) + mat_to_beta(m1) * mat_cube(mat_to_parR(m1)) + mat_to_beta(m2) * mat_cube(mat_to_parR(m2)) + mat_to_beta(m3) * mat_cube(mat_to_parR(m3)) + mat_to_beta(m4) * mat_cube(mat_to_parR(m4)) + mat_to_beta(m5) * mat_cube(mat_to_parR(m5)) + mat_to_beta(m6) * mat_cube(mat_to_parR(m6)) + mat_to_beta(m7) * mat_cube(mat_to_parR(m7))) / mat_dv_min(mat_to_beta(m0) + mat_to_beta(m1) + mat_to_beta(m2) + mat_to_beta(m3) + mat_to_beta(m4) + mat_to_beta(m5) + mat_to_beta(m6) + mat_to_beta(m7)))) |
			parG_to_mat(mat_cbrt((mat_to_beta(m0) * mat_cube(mat_to_parG(m0)) + mat_to_beta(m1) * mat_cube(mat_to_parG(m1)) + mat_to_beta(m2) * mat_cube(mat_to_parG(m2)) + mat_to_beta(m3) * mat_cube(mat_to_parG(m3)) + mat_to_beta(m4) * mat_cube(mat_to_parG(m4)) + mat_to_beta(m5) * mat_cube(mat_to_parG(m5)) + mat_to_beta(m6) * mat_cube(mat_to_parG(m6)) + mat_to_beta(m7) * mat_cube(mat_to_parG(m7))) / mat_dv_min(mat_to_beta(m0) + mat_to_beta(m1) + mat_to_beta(m2) + mat_to_beta(m3) + mat_to_beta(m4) + mat_to_beta(m5) + mat_to_beta(m6) + mat_to_beta(m7)))) |
			parB_to_mat(mat_cbrt((mat_to_beta(m0) * mat_cube(mat_to_parB(m0)) + mat_to_beta(m1) * mat_cube(mat_to_parB(m1)) + mat_to_beta(m2) * mat_cube(mat_to_parB(m2)) + mat_to_beta(m3) * mat_cube(mat_to_parB(m3)) + mat_to_beta(m4) * mat_cube(mat_to_parB(m4)) + mat_to_beta(m5) * mat_cube(mat_to_parB(m5)) + mat_to_beta(m6) * mat_cube(mat_to_parB(m6)) + mat_to_beta(m7) * mat_cube(mat_to_parB(m7))) / mat_dv_min(mat_to_beta(m0) + mat_to_beta(m1) + mat_to_beta(m2) + mat_to_beta(m3) + mat_to_beta(m4) + mat_to_beta(m5) + mat_to_beta(m6) + mat_to_beta(m7)))) |
			beta_to_mat(mat_cbrt((mat_cube(mat_to_beta(m0)) + mat_cube(mat_to_beta(m1)) + mat_cube(mat_to_beta(m2)) + mat_cube(mat_to_beta(m3)) + mat_cube(mat_to_beta(m4)) + mat_cube(mat_to_beta(m5)) + mat_cube(mat_to_beta(m6)) + mat_cube(mat_to_beta(m7))) >> 3)) |
			medR_to_mat(mat_cbrt(((63 - mat_to_beta(m0)) * mat_cube(mat_to_medR(m0)) + (63 - mat_to_beta(m1)) * mat_cube(mat_to_medR(m1)) + (63 - mat_to_beta(m2)) * mat_cube(mat_to_medR(m2)) + (63 - mat_to_beta(m3)) * mat_cube(mat_to_medR(m3)) + (63 - mat_to_beta(m4)) * mat_cube(mat_to_medR(m4)) + (63 - mat_to_beta(m5)) * mat_cube(mat_to_medR(m5)) + (63 - mat_to_beta(m6)) * mat_cube(mat_to_medR(m6)) + (63 - mat_to_beta(m7)) * mat_cube(mat_to_medR(m7))) / mat_dv_min(504 - mat_to_beta(m0) - mat_to_beta(m1) - mat_to_beta(m2) - mat_to_beta(m3) - mat_to_beta(m4) - mat_to_beta(m5) - mat_to_beta(m6) - mat_to_beta(m7)))) |
			medG_to_mat(mat_cbrt(((63 - mat_to_beta(m0)) * mat_cube(mat_to_medG(m0)) + (63 - mat_to_beta(m1)) * mat_cube(mat_to_medG(m1)) + (63 - mat_to_beta(m2)) * mat_cube(mat_to_medG(m2)) + (63 - mat_to_beta(m3)) * mat_cube(mat_to_medG(m3)) + (63 - mat_to_beta(m4)) * mat_cube(mat_to_medG(m4)) + (63 - mat_to_beta(m5)) * mat_cube(mat_to_medG(m5)) + (63 - mat_to_beta(m6)) * mat_cube(mat_to_medG(m6)) + (63 - mat_to_beta(m7)) * mat_cube(mat_to_medG(m7))) / mat_dv_min(504 - mat_to_beta(m0) - mat_to_beta(m1) - mat_to_beta(m2) - mat_to_beta(m3) - mat_to_beta(m4) - mat_to_beta(m5) - mat_to_beta(m6) - mat_to_beta(m7)))) |
			medB_to_mat(mat_cbrt(((63 - mat_to_beta(m0)) * mat_cube(mat_to_medB(m0)) + (63 - mat_to_beta(m1)) * mat_cube(mat_to_medB(m1)) + (63 - mat_to_beta(m2)) * mat_cube(mat_to_medB(m2)) + (63 - mat_to_beta(m3)) * mat_cube(mat_to_medB(m3)) + (63 - mat_to_beta(m4)) * mat_cube(mat_to_medB(m4)) + (63 - mat_to_beta(m5)) * mat_cube(mat_to_medB(m5)) + (63 - mat_to_beta(m6)) * mat_cube(mat_to_medB(m6)) + (63 - mat_to_beta(m7)) * mat_cube(mat_to_medB(m7))) / mat_dv_min(504 - mat_to_beta(m0) - mat_to_beta(m1) - mat_to_beta(m2) - mat_to_beta(m3) - mat_to_beta(m4) - mat_to_beta(m5) - mat_to_beta(m6) - mat_to_beta(m7)))) |
			emit_to_mat((mat_to_emit(m0) + mat_to_emit(m1) + mat_to_emit(m2) + mat_to_emit(m3) + mat_to_emit(m4) + mat_to_emit(m5) + mat_to_emit(m6) + mat_to_emit(m7)) > 4));
	}
}