#pragma once

#include "../Types.hpp"
#include "../Perspective.hpp"

#include <vuk/RenderGraph.hpp>
#include <glm/mat4x4.hpp>
#include <array>

/*
	Cascaded shadows are a simple spin on regular shadow mapping:
	Split the shadow map into N z-slices. At each slice, render a closer shadow map (i.e. higher quality).
	This reduces the aliasing that comes with regular shadow maps.
*/

class CascadedShadowRenderPass {
  public:
	static constexpr u8 SHADOW_MAP_CASCADE_COUNT = 4;
	static constexpr u32 DIMENSION = 4096;

	struct CascadeInfo {
		f32 split_depth;
		glm::mat4 view_proj_mat;
	};

	static void setup(struct Context& ctxt);

	CascadedShadowRenderPass();

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt);
	void debug(vuk::CommandBuffer& cbuf, u8 cascade);
	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, const class SceneRenderer& renderer);
	std::array<CascadeInfo, SHADOW_MAP_CASCADE_COUNT> compute_cascades();
	vuk::ImageView shadow_map_view() const;

	f32 cascade_split_lambda;

	glm::vec3 light_direction;

	Perspective cam_proj;
	glm::mat4 cam_view;
	f32 cam_near;
	f32 cam_far;

  private:
	std::vector<std::string> m_attachment_names;
	std::vector<vuk::Unique<vuk::ImageView>> m_image_views;

	vuk::Texture m_shadow_map;
	vuk::Unique<vuk::ImageView> m_shadow_map_view;
};
