#pragma once

#include "mat4.h"

#include <functional>

class Camera
{
public:

	Camera(std::function<void(mat4)> render);
	~Camera();

	void render();

private:

	float x, y, z, r, p, a;

	std::function<void(mat4)> renderFunction;
};