#include "Perspective.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Perspective::matrix(bool flip) const {
	auto m = glm::perspective(fovy, aspect_ratio, near, far);
	m[1][1] *= flip ? -1.f : 1;
	return m;
}

f32 Perspective::fovx() const {
	// the regular formula of multiplying fovy by aspect to get fovx is not accurate.
	return 2.f * glm::atan(glm::tan(fovy * 0.5f) * aspect_ratio);
}
