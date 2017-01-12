#include "mat4.h"

mat4::mat4(double v11, double v12, double v13, double v14, double v21, double v22, double v23, double v24, double v31, double v32, double v33, double v34, double v41, double v42, double v43, double v44) : matrix{ v11, v12, v13, v14, v21, v22, v23, v24, v31, v32, v33, v34, v41, v42, v43, v44 } {
	
}

mat4::mat4() : mat4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) {

}

mat4::~mat4() {

}

mat4 mat4::operator*(const mat4& m) {
	return mat4(matrix[0] * m.matrix[0] + matrix[1] * m.matrix[4] + matrix[2] * m.matrix[8] + matrix[3] * m.matrix[12],
		matrix[0] * m.matrix[1] + matrix[1] * m.matrix[5] + matrix[2] * m.matrix[9] + matrix[3] * m.matrix[13],
		matrix[0] * m.matrix[2] + matrix[1] * m.matrix[6] + matrix[2] * m.matrix[10] + matrix[3] * m.matrix[14],
		matrix[0] * m.matrix[3] + matrix[1] * m.matrix[7] + matrix[2] * m.matrix[11] + matrix[3] * m.matrix[15],
		matrix[4] * m.matrix[0] + matrix[5] * m.matrix[4] + matrix[6] * m.matrix[8] + matrix[7] * m.matrix[12],
		matrix[4] * m.matrix[1] + matrix[5] * m.matrix[5] + matrix[6] * m.matrix[9] + matrix[7] * m.matrix[13],
		matrix[4] * m.matrix[2] + matrix[5] * m.matrix[6] + matrix[6] * m.matrix[10] + matrix[7] * m.matrix[14],
		matrix[4] * m.matrix[3] + matrix[5] * m.matrix[7] + matrix[6] * m.matrix[11] + matrix[7] * m.matrix[15],
		matrix[8] * m.matrix[0] + matrix[9] * m.matrix[4] + matrix[10] * m.matrix[8] + matrix[11] * m.matrix[12],
		matrix[8] * m.matrix[1] + matrix[9] * m.matrix[5] + matrix[10] * m.matrix[9] + matrix[11] * m.matrix[13],
		matrix[8] * m.matrix[2] + matrix[9] * m.matrix[6] + matrix[10] * m.matrix[10] + matrix[11] * m.matrix[14],
		matrix[8] * m.matrix[3] + matrix[9] * m.matrix[7] + matrix[10] * m.matrix[11] + matrix[11] * m.matrix[15],
		matrix[12] * m.matrix[0] + matrix[13] * m.matrix[4] + matrix[14] * m.matrix[8] + matrix[15] * m.matrix[12],
		matrix[12] * m.matrix[1] + matrix[13] * m.matrix[5] + matrix[14] * m.matrix[9] + matrix[15] * m.matrix[13],
		matrix[12] * m.matrix[2] + matrix[13] * m.matrix[6] + matrix[14] * m.matrix[10] + matrix[15] * m.matrix[14],
		matrix[12] * m.matrix[3] + matrix[13] * m.matrix[7] + matrix[14] * m.matrix[11] + matrix[15] * m.matrix[15]);
}

mat4 mat4::inverse() {
	mat4 inv = mat4(matrix[5] * matrix[10] * matrix[15] - matrix[5] * matrix[11] * matrix[14] - matrix[9] * matrix[6] * matrix[15] + matrix[9] * matrix[7] * matrix[14] + matrix[13] * matrix[6] * matrix[11] - matrix[13] * matrix[7] * matrix[10],
		-matrix[1] * matrix[10] * matrix[15] + matrix[1] * matrix[11] * matrix[14] + matrix[9] * matrix[2] * matrix[15] - matrix[9] * matrix[3] * matrix[14] - matrix[13] * matrix[2] * matrix[11] + matrix[13] * matrix[3] * matrix[10],
		matrix[1] * matrix[6] * matrix[15] - matrix[1] * matrix[7] * matrix[14] - matrix[5] * matrix[2] * matrix[15] + matrix[5] * matrix[3] * matrix[14] + matrix[13] * matrix[2] * matrix[7] - matrix[13] * matrix[3] * matrix[6],
		-matrix[1] * matrix[6] * matrix[11] + matrix[1] * matrix[7] * matrix[10] + matrix[5] * matrix[2] * matrix[11] - matrix[5] * matrix[3] * matrix[10] - matrix[9] * matrix[2] * matrix[7] + matrix[9] * matrix[3] * matrix[6],
		-matrix[4] * matrix[10] * matrix[15] + matrix[4] * matrix[11] * matrix[14] + matrix[8] * matrix[6] * matrix[15] - matrix[8] * matrix[7] * matrix[14] - matrix[12] * matrix[6] * matrix[11] + matrix[12] * matrix[7] * matrix[10],
		matrix[0] * matrix[10] * matrix[15] - matrix[0] * matrix[11] * matrix[14] - matrix[8] * matrix[2] * matrix[15] + matrix[8] * matrix[3] * matrix[14] + matrix[12] * matrix[2] * matrix[11] - matrix[12] * matrix[3] * matrix[10],
		-matrix[0] * matrix[6] * matrix[15] + matrix[0] * matrix[7] * matrix[14] + matrix[4] * matrix[2] * matrix[15] - matrix[4] * matrix[3] * matrix[14] - matrix[12] * matrix[2] * matrix[7] + matrix[12] * matrix[3] * matrix[6],
		matrix[0] * matrix[6] * matrix[11] - matrix[0] * matrix[7] * matrix[10] - matrix[4] * matrix[2] * matrix[11] + matrix[4] * matrix[3] * matrix[10] + matrix[8] * matrix[2] * matrix[7] - matrix[8] * matrix[3] * matrix[6],
		matrix[4] * matrix[9] * matrix[15] - matrix[4] * matrix[11] * matrix[13] - matrix[8] * matrix[5] * matrix[15] + matrix[8] * matrix[7] * matrix[13] + matrix[12] * matrix[5] * matrix[11] - matrix[12] * matrix[7] * matrix[9],
		-matrix[0] * matrix[9] * matrix[15] + matrix[0] * matrix[11] * matrix[13] + matrix[8] * matrix[1] * matrix[15] - matrix[8] * matrix[3] * matrix[13] - matrix[12] * matrix[1] * matrix[11] + matrix[12] * matrix[3] * matrix[9],
		matrix[0] * matrix[5] * matrix[15] - matrix[0] * matrix[7] * matrix[13] - matrix[4] * matrix[1] * matrix[15] + matrix[4] * matrix[3] * matrix[13] + matrix[12] * matrix[1] * matrix[7] - matrix[12] * matrix[3] * matrix[5],
		-matrix[0] * matrix[5] * matrix[11] + matrix[0] * matrix[7] * matrix[9] + matrix[4] * matrix[1] * matrix[11] - matrix[4] * matrix[3] * matrix[9] - matrix[8] * matrix[1] * matrix[7] + matrix[8] * matrix[3] * matrix[5],
		-matrix[4] * matrix[9] * matrix[14] + matrix[4] * matrix[10] * matrix[13] + matrix[8] * matrix[5] * matrix[14] - matrix[8] * matrix[6] * matrix[13] - matrix[12] * matrix[5] * matrix[10] + matrix[12] * matrix[6] * matrix[9],
		matrix[0] * matrix[9] * matrix[14] - matrix[0] * matrix[10] * matrix[13] - matrix[8] * matrix[1] * matrix[14] + matrix[8] * matrix[2] * matrix[13] + matrix[12] * matrix[1] * matrix[10] - matrix[12] * matrix[2] * matrix[9],
		-matrix[0] * matrix[5] * matrix[14] + matrix[0] * matrix[6] * matrix[13] + matrix[4] * matrix[1] * matrix[14] - matrix[4] * matrix[2] * matrix[13] - matrix[12] * matrix[1] * matrix[6] + matrix[12] * matrix[2] * matrix[5],
		matrix[0] * matrix[5] * matrix[10] - matrix[0] * matrix[6] * matrix[9] - matrix[4] * matrix[1] * matrix[10] + matrix[4] * matrix[2] * matrix[9] + matrix[8] * matrix[1] * matrix[6] - matrix[8] * matrix[2] * matrix[5]);

	double det = matrix[0] * inv.matrix[0] + matrix[1] * inv.matrix[4] + matrix[2] * inv.matrix[8] + matrix[3] * inv.matrix[12];

	if (det == 0) {
		return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	}

	det = 1.0 / det;

	for (int i = 0; i < 16; ++i) {
		inv.matrix[i] *= det;
	}

	return inv;
}