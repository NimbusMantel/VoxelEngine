#pragma once

#include <functional>

class Camera
{
public:

	Camera(std::function<void()> render);
	~Camera();

	void render();

private:

	std::function<void()> renderFunction;
};