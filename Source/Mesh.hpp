#pragma once

#include "Types.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <vector>
#include <utility>
#include <cmath>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

using Mesh = std::pair<std::vector<Vertex>, std::vector<u32>>;

Mesh generate_quad();
Mesh generate_cube();
Mesh generate_sphere();
