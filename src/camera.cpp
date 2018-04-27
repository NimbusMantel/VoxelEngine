#include "camera.hpp"

#include "glm/gtx/quaternion.hpp"

struct Camera {
	glm::vec3 position = glm::vec3(0.0f);
	glm::mat3 rotation = glm::mat3(1.0f);

	glm::vec3 target = glm::vec3(0.0f);

	float radius = 0.0f;

	bool isDirty = true;
} cam;

namespace camera {
	void mov(glm::vec3 dp) {
		cam.isDirty = true;

		dp = cam.rotation * dp;

		cam.position += dp;
		cam.target += dp;
	}

	void rot(glm::vec2 dr) {
		cam.isDirty = true;

		glm::mat3 rub = glm::transpose(cam.rotation);
		rub = glm::mat3(glm::normalize(rub[0]), glm::normalize(rub[1]), glm::normalize(rub[2]));

		glm::vec3 dir = rub[0] * dr.x + rub[1] * dr.y;

		glm::mat3 rm = glm::toMat3(glm::angleAxis(glm::length(dir), glm::normalize(glm::cross(rub[2], dir))));
		
		cam.rotation = rm * glm::transpose(rub);
		cam.position = cam.target + glm::transpose(cam.rotation)[2] * cam.radius;
	}

	void rad(float dr) {
		cam.isDirty = true;

		cam.radius = fmax(0.1f, cam.radius + dr);

		cam.position = cam.target + glm::transpose(cam.rotation)[2] * cam.radius;
	}

	bool upd(glm::vec3& pos, glm::mat3& mat) {
		if (cam.isDirty) {
			pos = cam.position;
			mat = cam.rotation;

			cam.isDirty = false;

			return true;
		}

		return false;
	}
}