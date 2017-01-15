#pragma once

#include "geo.h"

#include <functional>

class Camera
{
public:

	Camera(std::function<void(mat4)> render);
	~Camera();

	void render();

	void setPositionX(float x);
	void setPositionY(float y);
	void setPositionZ(float z);

	void setRotationRoll(float r);
	void setRotationPitch(float p);
	void setRotationYaw(float y);

	float getPositionX();
	float getPositionY();
	float getPositionZ();

	float getRotationRoll();
	float getRotationPitch();
	float getRotationYaw();

private:

	float x, y, z, r, p, a;

	std::function<void(mat4)> renderFunction;
};