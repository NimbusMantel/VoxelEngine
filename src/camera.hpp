#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace camera {
	void mov(glm::vec3 dp);
	void rot(glm::vec2 dr);

	void rad(float dr);

	bool upd(glm::vec3& pos, glm::mat3& mat);
}