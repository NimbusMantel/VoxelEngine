#pragma once

#include <cstring>

#include <cmath>

#include "host/macros.hpp"

namespace camera {
	enum Mode {
		FREE,
		ARCBALL
	};

	void mod(Mode m);
	Mode mod();
	
	void mov(float dx, float dy, float dz);
	void rot(float dx, float dy);

	void rad(float dr);

	void pos(float& x, float& y, float& z);
	void mat(float* m);
}