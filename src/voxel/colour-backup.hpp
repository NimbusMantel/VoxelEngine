#define COL_TO_PAR(c)		((c & 0xFFFFFF0000000000) >> 40)
#define PAR_TO_COL(c)		((c & 0x0000000000FFFFFF) << 40)

#define COL_TO_BET(c)		((c & 0x000000FF00000000) >> 32)
#define BET_TO_COL(c)		((c & 0x00000000000000FF) << 32)

#define COL_TO_MED(c)		((c & 0x00000000FFFFFF00) >> 8)
#define MED_TO_COL(c)		((c & 0x0000000000FFFFFF) << 8)

#define RGB_TO_RED(c)		((c & 0xFF0000) >> 16)
#define RGB_TO_GRE(c)		((c & 0x00FF00) >> 8)
#define RGB_TO_BLU(c)		(c & 0x0000FF)

#define RGB_TO_RGB(r, g, b) ((r << 16) | (g << 8) | b)

#define COL_SQRE(c)			(c * c)
#define COL_SQRT(c)			Colour::sqrt(c)

#define COL_DV_MIN(c)		(c + (c == 0))

#define COL_PL_COL(a, b)	PAR_TO_COL(RGB_TO_RGB( \
								COL_SQRT((COL_TO_BET(a) * COL_SQRE(RGB_TO_RED(COL_TO_PAR(a))) + COL_TO_BET(b) * COL_SQRE(RGB_TO_RED(COL_TO_PAR(b)))) / COL_DV_MIN(COL_TO_BET(a) + COL_TO_BET(b) + 0x01)), \
								COL_SQRT((COL_TO_BET(a) * COL_SQRE(RGB_TO_GRE(COL_TO_PAR(a))) + COL_TO_BET(b) * COL_SQRE(RGB_TO_GRE(COL_TO_PAR(b)))) / COL_DV_MIN(COL_TO_BET(a) + COL_TO_BET(b) + 0x01)), \
								COL_SQRT((COL_TO_BET(a) * COL_SQRE(RGB_TO_BLU(COL_TO_PAR(a))) + COL_TO_BET(b) * COL_SQRE(RGB_TO_BLU(COL_TO_PAR(b)))) / COL_DV_MIN(COL_TO_BET(a) + COL_TO_BET(b) + 0x01)) \
							)) | \
							BET_TO_COL((COL_TO_BET(a) + COL_TO_BET(b)) >> 1) | \
							MED_TO_COL(RGB_TO_RGB( \
								COL_SQRT(((0xFF - COL_TO_BET(a)) * COL_SQRE(RGB_TO_RED(COL_TO_MED(a))) + (0xFF - COL_TO_BET(b)) * COL_SQRE(RGB_TO_RED(COL_TO_MED(b)))) / COL_DV_MIN(0x01FF - COL_TO_BET(a) - COL_TO_BET(b))), \
								COL_SQRT(((0xFF - COL_TO_BET(a)) * COL_SQRE(RGB_TO_GRE(COL_TO_MED(a))) + (0xFF - COL_TO_BET(b)) * COL_SQRE(RGB_TO_GRE(COL_TO_MED(b)))) / COL_DV_MIN(0x01FF - COL_TO_BET(a) - COL_TO_BET(b))), \
								COL_SQRT(((0xFF - COL_TO_BET(a)) * COL_SQRE(RGB_TO_BLU(COL_TO_MED(a))) + (0xFF - COL_TO_BET(b)) * COL_SQRE(RGB_TO_BLU(COL_TO_MED(b)))) / COL_DV_MIN(0x01FF - COL_TO_BET(a) - COL_TO_BET(b))) \
							))

#define COL_OV_COL(a, b)	PAR_TO_COL(RGB_TO_RGB( \
								(((COL_TO_BET(a) * RGB_TO_RED(COL_TO_PAR(a))) >> 8) + (RGB_TO_RED(COL_TO_MED(a)) * (0xFF - COL_TO_BET(a)) * COL_TO_BET(b) * RGB_TO_RED(COL_TO_PAR(b))) >> 16), \
								(((COL_TO_BET(a) * RGB_TO_GRE(COL_TO_PAR(a))) >> 8) + (RGB_TO_GRE(COL_TO_MED(a)) * (0xFF - COL_TO_BET(a)) * COL_TO_BET(b) * RGB_TO_GRE(COL_TO_PAR(b))) >> 16), \
								(((COL_TO_BET(a) * RGB_TO_BLU(COL_TO_PAR(a))) >> 8) + (RGB_TO_BLU(COL_TO_MED(a)) * (0xFF - COL_TO_BET(a)) * COL_TO_BET(b) * RGB_TO_BLU(COL_TO_PAR(b))) >> 16) \
							)) | \
							BET_TO_COL(COL_TO_BET(a) + (((0xFF - COL_TO_BET(a)) * COL_TO_BET(b)) >> 8)) | \
							MED_TO_COL(RGB_TO_RGB( \
								((RGB_TO_RED(COL_TO_MED(a)) * RGB_TO_RED(COL_TO_MED(b))) >> 8), \
								((RGB_TO_GRE(COL_TO_MED(a)) * RGB_TO_GRE(COL_TO_MED(b))) >> 8), \
								((RGB_TO_BLU(COL_TO_MED(a)) * RGB_TO_BLU(COL_TO_MED(b))) >> 8) \
							))

#define COL_TO_PIX(c, f, b) RGB_TO_RGB( \
								COL_SQRT(((COL_TO_BET(c) * COL_SQRE((RGB_TO_RED(COL_TO_PAR(c)) * RGB_TO_RED(f)) >> 8)) + ((0xFF - COL_TO_BET(c)) * COL_SQRE((RGB_TO_RED(COL_TO_MED(c)) * RGB_TO_RED(b)) >> 8))) >> 8), \
								COL_SQRT(((COL_TO_BET(c) * COL_SQRE((RGB_TO_GRE(COL_TO_PAR(c)) * RGB_TO_GRE(f)) >> 8)) + ((0xFF - COL_TO_BET(c)) * COL_SQRE((RGB_TO_GRE(COL_TO_MED(c)) * RGB_TO_GRE(b)) >> 8))) >> 8), \
								COL_SQRT(((COL_TO_BET(c) * COL_SQRE((RGB_TO_BLU(COL_TO_PAR(c)) * RGB_TO_BLU(f)) >> 8)) + ((0xFF - COL_TO_BET(c)) * COL_SQRE((RGB_TO_BLU(COL_TO_MED(c)) * RGB_TO_BLU(b)) >> 8))) >> 8) \
							)