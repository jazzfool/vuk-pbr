#pragma once

/*
	Lights with volumetric fog.

	The basic idea is to start at each pixel position (sampled from the g-buffer) and raymarch towards the camera.
	At each step in the raymarch, check if the position is visible to the light. The shadow map can be leveraged for this.
*/

#include "GraphicsPass.hpp"
#include "CascadedShadows.hpp"
#include "../Mesh.hpp"
#include "../Perspective.hpp"

#include <vuk/Image.hpp>

namespace vuk {
struct RenderGraph;
class PerThreadContext;
} // namespace vuk

class VolumetricLightPass : public GraphicsPass {
  public:
	void debug(vuk::CommandBuffer& cbuf);

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt, struct UniformStore& uniforms, class PipelineStore& ps) override;
	void prep(vuk::PerThreadContext& ptc, struct Context& ctxt, struct RenderInfo& info) override;
	void render(vuk::PerThreadContext& ptc, struct Context& ctxt, vuk::RenderGraph& rg, const class SceneRenderer& renderer, struct RenderInfo& info) override;

  private:
	u32 m_width, m_height;
	vuk::Buffer m_verts;
	vuk::Buffer m_inds;
};
