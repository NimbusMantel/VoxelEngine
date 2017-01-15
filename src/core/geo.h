#pragma once

#include <cstddef>

class vec2
{
public:

	double x;
	double y;

};

class vec3
{
public:

	double x;
	double y;
	double z;

};

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
	const double& operator[](std::size_t idx) const { return matrix[idx]; }

	mat4 inverse();

	void log();

private:

	double matrix[16];
};