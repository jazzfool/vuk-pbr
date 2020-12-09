#pragma once

#include "../Types.hpp"

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

	struct CascadeInfo {
		f32 split_depth;
		glm::mat4 view_proj_mat;
	};

	static void setup(struct Context& ctxt);
	static void debug_shadow_map(vuk::CommandBuffer& cbuf, const vuk::ImageView& shadow_map, const struct RenderMesh& quad, u8 cascade);

	CascadedShadowRenderPass();

	std::array<vuk::Pass, SHADOW_MAP_CASCADE_COUNT> build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, vuk::Image out_depths);
	std::array<CascadeInfo, SHADOW_MAP_CASCADE_COUNT> compute_cascades();

	f32 cascade_split_lambda;

	glm::vec3 light_direction;

	glm::mat4 cam_proj;
	glm::mat4 cam_view;
	f32 cam_near;
	f32 cam_far;

	std::vector<struct RenderMesh*> meshes;
	vuk::Buffer transform_buffer;
	u32 transform_buffer_alignment;

  private:
	std::vector<std::string> m_attachment_names;
	std::vector<vuk::Unique<vuk::ImageView>> m_image_views;
};
