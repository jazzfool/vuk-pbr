#pragma once

/*
	Lights with volumetric fog.

	The basic idea is to start at each pixel position (sampled from the g-buffer) and raymarch towards the camera.
	At each step in the raymarch, check if the position is visible to the light. The shadow map can be leveraged for this.
*/

#include "CascadedShadows.hpp"
#include "../Mesh.hpp"
#include "../Perspective.hpp"

#include <vuk/Image.hpp>

namespace vuk {
struct RenderGraph;
class PerThreadContext;
} // namespace vuk

class VolumetricLightPass {
  public:
	static void setup(struct Context& ctxt);
	static void debug(vuk::CommandBuffer& cbuf);

	void init(struct Context& ctxt, vuk::PerThreadContext& ptc);
	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg);

	u32 width, height;

	Perspective cam_proj;
	glm::mat4 cam_view;

	vuk::ImageView shadow_map;
	glm::vec3 light_direction;
	glm::vec3 cam_pos;
	glm::vec3 cam_forward;
	glm::vec3 cam_up;
	std::array<CascadedShadowRenderPass::CascadeInfo, CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT> cascades;

  private:
	vuk::Buffer m_verts;
	vuk::Buffer m_inds;
};
