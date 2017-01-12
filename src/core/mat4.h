#pragma once

class mat4
{
public:

	mat4(double v11, double v12, double v13, double v14,
		double v21, double v22, double v23, double v24,
		double v31, double v32, double v33, double v34,
		double v41, double v42, double v43, double v44);

	mat4();

	~mat4();

	mat4 operator*(const mat4& m);

	mat4 inverse();

private:

	double matrix[16];
};