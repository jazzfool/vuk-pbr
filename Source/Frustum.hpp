#pragma once

#include "Types.hpp"

#include <glm/matrix.hpp>

class Frustum {
  public:
	Frustum() {
	}

	Frustum(glm::mat4 m);

	bool is_box_visible(const glm::vec3& minp, const glm::vec3& maxp) const;

  private:
	enum Planes { Left = 0, Right, Bottom, Top, Near, Far, Count, Combinations = Count * (Count - 1) / 2 };

	template <Planes i, Planes j>
	struct ij2k {
		static constexpr i32 k = i * (9 - i) / 2 + j - 1;
	};

	template <Planes a, Planes b, Planes c>
	glm::vec3 intersection(const glm::vec3* crosses) const;

	glm::vec4 m_planes[Count];
	glm::vec3 m_points[8];
};

template <Frustum::Planes a, Frustum::Planes b, Frustum::Planes c>
glm::vec3 Frustum::intersection(const glm::vec3* crosses) const {
	float D = glm::dot(glm::vec3(m_planes[a]), crosses[ij2k<b, c>::k]);
	glm::vec3 res = glm::mat3(crosses[ij2k<b, c>::k], -crosses[ij2k<a, c>::k], crosses[ij2k<a, b>::k]) * glm::vec3(m_planes[a].w, m_planes[b].w, m_planes[c].w);
	return res * (-1.0f / D);
}
