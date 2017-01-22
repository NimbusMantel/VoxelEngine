#pragma once

#include "geo.h"

#include <functional>

class Camera
{
public:

	Camera(std::function<void(mat4)> render);
	Camera() : Camera([](mat4) {return; }) {};

	~Camera();

	void render();

	void setPositionX(float x);
	void setPositionY(float y);
	void setPositionZ(float z);

	void setRotationX(float x);
	void setRotationY(float y);
	void setRotationZ(float z);

	float getPositionX();
	float getPositionY();
	float getPositionZ();

	float getRotationX();
	float getRotationY();
	float getRotationZ();

private:

	float x, y, z, r, p, a;

	std::function<void(mat4)> renderFunction;
};