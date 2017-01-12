#include "camera.h"

#include "mat4.h"

#include <math.h>

Camera::Camera(std::function<void(mat4)> r) : x(10), y(0), z(-5), r(0), p(1), a(0), renderFunction(r) {

}

Camera::~Camera() {

}

void Camera::render() {
	mat4 matrix = mat4(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1)
				* mat4(1, 0, 0, 0, 0, cosf(r), -sinf(r), 0, 0, sinf(r), cosf(r), 0, 0, 0, 0, 1)
				* mat4(cosf(p), 0, sinf(p), 0, 0, 1, 0, 0, -sinf(p), 0, cosf(p), 0, 0, 0, 0, 1)
				* mat4(cosf(a), -sinf(a), 0, 0, sinf(a), cosf(a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	renderFunction(matrix.inverse());
}