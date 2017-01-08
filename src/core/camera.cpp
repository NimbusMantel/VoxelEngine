#include "camera.h"

Camera::Camera(std::function<void()> r) :renderFunction(r) {

}

Camera::~Camera() {

}

void Camera::render() {
	renderFunction();
}