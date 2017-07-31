#include "camera.hpp"

#include <iostream>

camera::Mode mode = camera::Mode::ARCBALL;

float position[3] = { 0.5f, 0.5f, 0.5f };
float rotation[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

float target[3] = { 0.5f, 0.5f, 0.5f };

float radius = 0.0f;

void axisAngle(float* x, float a, float* m) {
	float c = cosf(a);
	float s = sinf(a);
	float t = 1.0f - c;

	m[0] = t * x[0] * x[0] + c;
	m[1] = t * x[0] * x[1] - x[2] * s;
	m[2] = t * x[0] * x[2] + x[1] * s;
	m[3] = t * x[0] * x[1] + x[2] * s;
	m[4] = t * x[1] * x[1] + c;
	m[5] = t * x[1] * x[2] - x[0] * s;
	m[6] = t * x[0] * x[2] - x[1] * s;
	m[7] = t * x[1] * x[2] + x[0] * s;
	m[8] = t * x[2] * x[2] + c;
}

namespace camera {
	void mod(Mode m) {
		if (m == Mode::FREE && mode != Mode::FREE) {
			// pass
		}
		else if (m == Mode::ARCBALL && mode != Mode::ARCBALL) {
			radius = 0.0f;
			
			target[0] = position[0];
			target[1] = position[1];
			target[2] = position[2];
		}

		mode = m;
	}

	Mode mod() {
		return mode;
	}
	
	void mov(float dx, float dy, float dz) {
		float dX = rotation[0] * dx + rotation[1] * dy - rotation[2] * dz;
		float dY = rotation[3] * dx + rotation[4] * dy - rotation[5] * dz;
		float dZ = rotation[6] * dx + rotation[7] * dy - rotation[8] * dz;

		position[0] += dX; position[1] += dY; position[2] += dZ;
		target[0] += dX; target[1] += dY; target[2] += dZ;
	}

	void rot(float dx, float dy) {
		dx *= -M_PI;
		dy *= -M_PI;

		float right[3] = { rotation[0], rotation[3], rotation[6] };
		float up[3] = { rotation[1], rotation[4], rotation[7] };
		float back[3] = { rotation[2], rotation[5], rotation[8] };

		float mul = 1.0f / sqrtf(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
		right[0] *= mul; right[1] *= mul; right[2] *= mul;

		mul = 1.0f / sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
		up[0] *= mul; up[1] *= mul; up[2] *= mul;

		mul = 1.0f / sqrtf(back[0] * back[0] + back[1] * back[1] + back[2] * back[2]);
		back[0] *= mul; back[1] *= mul; back[2] *= mul;

		float dir[3] = { right[0] * dx + up[0] * dy, right[1] * dx + up[1] * dy, right[2] * dx + up[2] * dy };
		
		float axi[3] = { back[1] * dir[2] - dir[1] * back[2], back[2] * dir[0] - dir[2] * back[0], back[0] * dir[1] - dir[0] * back[1] };
		float len = sqrtf(axi[0] * axi[0] + axi[1] * axi[1] + axi[2] * axi[2]);
		axi[0] /= len; axi[1] /= len; axi[2] /= len;

		float rm[9];
		len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
		axisAngle(axi, len, rm);

		float tmp[3];

		tmp[0] = rm[0] * right[0] + rm[1] * right[1] + rm[2] * right[2];
		tmp[1] = rm[3] * right[0] + rm[4] * right[1] + rm[5] * right[2];
		tmp[2] = rm[6] * right[0] + rm[7] * right[1] + rm[8] * right[2];

		memcpy(right, tmp, sizeof(float) * 3);

		tmp[0] = rm[0] * up[0] + rm[1] * up[1] + rm[2] * up[2];
		tmp[1] = rm[3] * up[0] + rm[4] * up[1] + rm[5] * up[2];
		tmp[2] = rm[6] * up[0] + rm[7] * up[1] + rm[8] * up[2];

		memcpy(up, tmp, sizeof(float) * 3);

		tmp[0] = rm[0] * back[0] + rm[1] * back[1] + rm[2] * back[2];
		tmp[1] = rm[3] * back[0] + rm[4] * back[1] + rm[5] * back[2];
		tmp[2] = rm[6] * back[0] + rm[7] * back[1] + rm[8] * back[2];

		memcpy(back, tmp, sizeof(float) * 3);

		rotation[0] = right[0]; rotation[3] = right[1]; rotation[6] = right[2];
		rotation[1] = up[0]; rotation[4] = up[1]; rotation[7] = up[2];
		rotation[2] = back[0]; rotation[5] = back[1]; rotation[8] = back[2];
		
		if (mode == camera::Mode::ARCBALL) {
			position[0] = target[0] + rotation[2] * radius;
			position[1] = target[1] + rotation[5] * radius;
			position[2] = target[2] + rotation[8] * radius;
		}
	}

	void rad(float dr) {
		radius = fmax(0.1f, radius + dr);

		if (mode == camera::Mode::ARCBALL) {
			position[0] = target[0] + rotation[2] * radius;
			position[1] = target[1] + rotation[5] * radius;
			position[2] = target[2] + rotation[8] * radius;
		}
	}

	void pos(float& x, float& y, float& z) {
		x = position[0];
		y = position[1];
		z = position[2];
	}

	void mat(float* m) {
		memcpy(m, (void*)rotation, sizeof(float) * 9);
	}
}