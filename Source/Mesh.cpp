#include "Mesh.hpp"

Mesh generate_quad() {
	return Mesh(
		std::vector<Vertex>{
			Vertex{{-1, 1, 0}, {0, 0, 1}, {0, 1}},
			Vertex{{-1, -1, 0}, {0, 0, 1}, {0, 0}},
			Vertex{{1, 1, 0}, {0, 0, 1}, {1, 1}},
			Vertex{{1, -1, 0}, {0, 0, 1}, {1, 0}},
		},
		{0, 1, 2, 1, 2, 3});
}

Mesh generate_cube() {
	// clang-format off
		return Mesh(std::vector<Vertex> { 
			// back
			Vertex{ {-1, -1, -1}, {0, 0, -1}, {1, 0} }, Vertex{ {1, 1, -1}, {0, 0, -1}, {0, 1} },
				Vertex{ {1, -1, -1}, {0, 0, -1}, {0, 0} }, Vertex{ {1, 1, -1}, {0, 0, -1}, {0, 1} },
				Vertex{ {-1, -1, -1}, {0, 0, -1}, {1, 0} }, Vertex{ {-1, 1, -1}, {0, 0, -1}, {1, 1} },
				// front 
				Vertex{ {-1, -1, 1}, {0, 0, 1}, {0, 0} }, Vertex{ {1, -1, 1}, {0, 0, 1}, {1, 0} },
				Vertex{ {1, 1, 1}, {0, 0, 1}, {1, 1} }, Vertex{ {1, 1, 1}, {0, 0, 1}, {1, 1} },
				Vertex{ {-1, 1, 1}, {0, 0, 1}, {0, 1} }, Vertex{ {-1, -1, 1}, {0, 0, 1}, {0, 0} },
				// left 
				Vertex{ {-1, 1, -1}, {-1, 0, 0}, {0, 1} }, Vertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0} },
				Vertex{ {-1, 1, 1}, {-1, 0, 0}, {1, 1} }, Vertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0} },
				Vertex{ {-1, -1, 1}, {-1, 0, 0}, {1, 0} }, Vertex{ {-1, 1, 1}, {-1, 0, 0}, {1, 1} },
				// right 
				Vertex{ {1, 1, 1}, {1, 0, 0}, {0, 1} }, Vertex{ {1, -1, -1}, {1, 0, 0}, {1, 0} },
				Vertex{ {1, 1, -1}, {1, 0, 0}, {1, 1} }, Vertex{ {1, -1, -1}, {1, 0, 0}, {1, 0} },
				Vertex{ {1, 1, 1}, {1, 0, 0}, {0, 1} }, Vertex{ {1, -1, 1}, {1, 0, 0},  {0, 0} },
				// bottom 
				Vertex{ {-1, -1, -1}, {0, -1, 0}, {0, 0} }, Vertex{ {1, -1, -1}, {0, -1, 0}, {1, 0} },
				Vertex{ {1, -1, 1}, {0, -1, 0}, {1, 1} }, Vertex{ {1, -1, 1}, {0, -1, 0}, {1, 1} },
				Vertex{ {-1, -1, 1}, {0, -1, 0}, {0, 1} }, Vertex{ {-1, -1, -1}, {0, -1, 0}, {0, 0} },
				// top 
				Vertex{ {-1, 1, -1}, {0, 1, 0}, {0, 1} }, Vertex{ {1, 1, 1}, {0, 1, 0}, {1, 0} },
				Vertex{ {1, 1, -1}, {0, 1, 0}, {1, 1} }, Vertex{ {1, 1, 1}, {0, 1, 0}, {1, 0} },
				Vertex{ {-1, 1, -1}, {0, 1, 0}, {0, 1} }, Vertex{ {-1, 1, 1}, {0, 1, 0}, {0, 0} } },
			{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 });
	// clang-format on
}

Mesh generate_sphere() {
	static constexpr u32 X_SEGMENTS = 64;
	static constexpr u32 Y_SEGMENTS = 64;
	static constexpr f32 PI = 3.14159265359;

	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	for (u32 y = 0; y <= Y_SEGMENTS; ++y) {
		for (u32 x = 0; x <= X_SEGMENTS; ++x) {
			f32 xSegment = (f32)x / (f32)X_SEGMENTS;
			f32 ySegment = (f32)y / (f32)Y_SEGMENTS;
			f32 xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			f32 yPos = std::cos(ySegment * PI);
			f32 zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			Vertex vert;
			vert.position = glm::vec3(xPos, yPos, zPos);
			vert.texCoord = glm::vec2(xSegment, ySegment);
			vert.normal = glm::vec3(xPos, yPos, zPos);

			vertices.push_back(vert);
		}
	}

	bool oddRow = false;
	for (u32 y = 0; y < Y_SEGMENTS; ++y) {
		if (!oddRow) {
			for (u32 x = 0; x <= X_SEGMENTS; ++x) {
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		} else {
			for (i32 x = X_SEGMENTS; x >= 0; --x) {
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	return std::make_pair(vertices, indices);
}
