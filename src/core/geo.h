#pragma once

#define M_PI 3.14159265359

#include <cstddef>

class vec2;
class vec3;
class vec4;
class mat4;

class vec2
{
public:

	vec2(double x, double y);
	vec2();

	~vec2();

	double x;
	double y;

};

class vec3
{
public:

	vec3(double x, double y, double z);
	vec3();

	~vec3();

	double x;
	double y;
	double z;

	operator vec2() const;

};

class vec4
{
public:

	vec4(double x, double y, double z, double w);
	vec4();

	~vec4();

	double x;
	double y;
	double z;
	double w;

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
	vec4 operator*(const vec4& v);
	vec3 operator*(const vec3& v);

	const double& operator[](std::size_t idx) const { return matrix[idx]; }

	mat4 inverse();

	void log();

private:

	double matrix[16];
};