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
	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, const struct RenderInfo& info);

	u32 width, height;

  private:
	vuk::Buffer m_verts;
	vuk::Buffer m_inds;
};
