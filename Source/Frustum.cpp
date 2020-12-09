#include "Frustum.hpp"

Frustum::Frustum(glm::mat4 m) {
	m = glm::transpose(m);
	m_planes[Left] = m[3] + m[0];
	m_planes[Right] = m[3] - m[0];
	m_planes[Bottom] = m[3] + m[1];
	m_planes[Top] = m[3] - m[1];
	m_planes[Near] = m[3] + m[2];
	m_planes[Far] = m[3] - m[2];

	glm::vec3 crosses[Combinations] = {
		glm::cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Right])),	glm::cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Bottom])),
		glm::cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Top])),	glm::cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Far])),	glm::cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Bottom])),
		glm::cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Top])),	glm::cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Far])),	glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Top])),
		glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Near])), glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Top]), glm::vec3(m_planes[Near])),	glm::cross(glm::vec3(m_planes[Top]), glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Near]), glm::vec3(m_planes[Far]))};

	m_points[0] = intersection<Left, Bottom, Near>(crosses);
	m_points[1] = intersection<Left, Top, Near>(crosses);
	m_points[2] = intersection<Right, Bottom, Near>(crosses);
	m_points[3] = intersection<Right, Top, Near>(crosses);
	m_points[4] = intersection<Left, Bottom, Far>(crosses);
	m_points[5] = intersection<Left, Top, Far>(crosses);
	m_points[6] = intersection<Right, Bottom, Far>(crosses);
	m_points[7] = intersection<Right, Top, Far>(crosses);
}

bool Frustum::is_box_visible(const glm::vec3& minp, const glm::vec3& maxp) const {
	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm

	for (int i = 0; i < Count; i++) {
		if ((glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, minp.z, 1.0f)) < 0.0) && (glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, minp.z, 1.0f)) < 0.0) && (glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, maxp.z, 1.0f)) < 0.0) && (glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, maxp.z, 1.0f)) < 0.0) && (glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, maxp.z, 1.0f)) < 0.0)) {
			return false;
		}
	}

	int out;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].x > maxp.x) ? 1 : 0);
	if (out == 8)
		return false;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].x < minp.x) ? 1 : 0);
	if (out == 8)
		return false;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].y > maxp.y) ? 1 : 0);
	if (out == 8)
		return false;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].y < minp.y) ? 1 : 0);
	if (out == 8)
		return false;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].z > maxp.z) ? 1 : 0);
	if (out == 8)
		return false;
	out = 0;
	for (int i = 0; i < 8; i++)
		out += ((m_points[i].z < minp.z) ? 1 : 0);
	if (out == 8)
		return false;

	return true;
}
