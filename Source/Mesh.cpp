#include "Mesh.hpp"

#include "Resource.hpp"
#include "Util.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/hash.hpp>
#include <vuk/Context.hpp>
#include <tiny_obj_loader.h>
#include <spdlog/spdlog.h>

bool Vertex::operator==(const Vertex& rhs) const {
	return position == rhs.position && normal == rhs.normal && tex_coord == rhs.tex_coord;
}

std::size_t VertexHash::operator()(const Vertex& v) const noexcept {
	std::size_t h = 0;
	hash_combine(h, v.position, v.normal, v.tex_coord);
	return h;
}

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

Mesh load_obj(std::string_view path) {
	auto res = get_resource(path);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	IMemoryStream ins{reinterpret_cast<const char*>(res.data), res.size};

	if (!tinyobj::LoadObj(&attrib, &shapes, nullptr, nullptr, nullptr, &ins, nullptr)) {
		spdlog::error("failed to load OBJ at {}", path);
	}

	Mesh mesh;

	std::unordered_map<Vertex, u32, VertexHash> unique_verts;

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vert;

			vert.position.x = attrib.vertices[3 * index.vertex_index];
			vert.position.y = attrib.vertices[3 * index.vertex_index + 1];
			vert.position.z = attrib.vertices[3 * index.vertex_index + 2];

			vert.normal.x = attrib.normals[3 * index.normal_index];
			vert.normal.y = attrib.normals[3 * index.normal_index + 1];
			vert.normal.z = attrib.normals[3 * index.normal_index + 2];

			vert.tex_coord.x = attrib.texcoords[2 * index.texcoord_index];
			vert.tex_coord.y = 1.f - attrib.texcoords[2 * index.texcoord_index + 1];

			if (!unique_verts.contains(vert)) {
				unique_verts[vert] = static_cast<u32>(mesh.first.size());
				mesh.first.push_back(vert);
			}

			mesh.second.push_back(unique_verts[vert]);
		}
	}

	return mesh;
}

void RenderMesh::upload(vuk::PerThreadContext& ptc) {
	auto [bverts, _s1] = ptc.create_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span(&mesh.first[0], mesh.first.size()));
	verts = std::move(bverts);
	auto [binds, _s2] = ptc.create_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span(&mesh.second[0], mesh.second.size()));
	inds = std::move(binds);
}

void RenderMesh::compute_bounds() {
	min = mesh.first[0].position;
	max = mesh.first[0].position;
	for (const auto& vert : mesh.first) {
		if (vert.position.x < min.x) {
			min.x = vert.position.x;
		}

		if (vert.position.y < min.y) {
			min.y = vert.position.y;
		}

		if (vert.position.z < min.z) {
			min.z = vert.position.z;
		}

		if (vert.position.x > max.x) {
			max.x = vert.position.x;
		}

		if (vert.position.y > max.y) {
			max.y = vert.position.y;
		}

		if (vert.position.z > max.z) {
			max.z = vert.position.z;
		}
	}
}

TransformComponent::TransformComponent() : matrix{1} {
}

TransformComponent& TransformComponent::translate(const glm::vec3& offset) {
	matrix = glm::translate(matrix, offset);
	return *this;
}

TransformComponent& TransformComponent::scale(const glm::vec3& scale) {
	matrix = glm::scale(matrix, scale);
	return *this;
}

TransformComponent& TransformComponent::rotate(const glm::quat& rot) {
	matrix *= glm::mat4_cast(rot);
	return *this;
}

DecomposedTransform TransformComponent::decompose() const {
	DecomposedTransform dt = {};
	glm::decompose(matrix, dt.scale, dt.rotation, dt.translation, dt.skew, dt.perspective);
	return dt;
}
