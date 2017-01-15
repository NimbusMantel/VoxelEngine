#include "camera.h"

#include <math.h>

Camera::Camera(std::function<void(mat4)> r) : x(0), y(0), z(0), r(0), p(0), a(0), renderFunction(r) {

}

Camera::~Camera() {

}

void Camera::render() {
	renderFunction(mat4(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1)
				 * mat4(1, 0, 0, 0, 0, cosf(r), -sinf(r), 0, 0, sinf(r), cosf(r), 0, 0, 0, 0, 1)
				 * mat4(cosf(p), 0, sinf(p), 0, 0, 1, 0, 0, -sinf(p), 0, cosf(p), 0, 0, 0, 0, 1)
				 * mat4(cosf(a), -sinf(a), 0, 0, sinf(a), cosf(a), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
}

void Camera::setPositionX(float p) {
	x = p;
}

void Camera::setPositionY(float p) {
	y = p;
}

void Camera::setPositionZ(float p) {
	z = p;
}

void Camera::setRotationRoll(float t) {
	r = t;
}

void Camera::setRotationPitch(float t) {
	p = t;
}

void Camera::setRotationYaw(float t) {
	a = t;
}

float Camera::getPositionX() {
	return x;
}

float Camera::getPositionY() {
	return y;
}

float Camera::getPositionZ() {
	return z;
}

float Camera::getRotationRoll() {
	return r;
}

float Camera::getRotationPitch() {
	return p;
}

float Camera::getRotationYaw() {
	return a;
}