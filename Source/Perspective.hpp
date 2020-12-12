#pragma once

#include "Types.hpp"

#include <glm/mat4x4.hpp>

struct Perspective {
	f32 fovy; // <- radians
	f32 aspect_ratio;
	f32 near;
	f32 far;

	glm::mat4 matrix(bool flip = true) const;
	f32 fovx() const;
};
