#pragma once

#include "Types.hpp"
#include "Cache.hpp"
#include "Material.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>
#include <entt/entt.hpp>
#include <vuk/Buffer.hpp>

#include <vector>
#include <utility>
#include <cmath>

namespace vuk {
class PerThreadContext;
}

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coord;

	bool operator==(const Vertex& rhs) const;
};

struct VertexHash {
	std::size_t operator()(const Vertex& v) const noexcept;
};

using Mesh = std::pair<std::vector<Vertex>, std::vector<u32>>;

Mesh generate_quad();
Mesh generate_cube();
Mesh load_obj(std::string_view path);

struct RenderMesh {
	void upload(vuk::PerThreadContext& ptc);
	void compute_bounds();

	Mesh mesh;
	glm::vec3 min;
	glm::vec3 max;
	vuk::Unique<vuk::Buffer> verts;
	vuk::Unique<vuk::Buffer> inds;
};

using MeshCache = Cache<std::string_view, RenderMesh>;

struct MeshComponent {
	MeshCache::View mesh;
	Material material;
};

struct DecomposedTransform {
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
};

struct TransformComponent {
	TransformComponent();

	TransformComponent& translate(const glm::vec3& offset);
	TransformComponent& scale(const glm::vec3& scale);
	TransformComponent& rotate(const glm::quat& rot);

	DecomposedTransform decompose() const;

	glm::mat4 matrix;
};
