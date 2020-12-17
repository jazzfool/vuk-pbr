#pragma once

#include "GraphicsPass.hpp"
#include "../Types.hpp"
#include "../Perspective.hpp"

#include <vuk/RenderGraph.hpp>
#include <glm/mat4x4.hpp>
#include <array>

/*
	Screen-space ambient occlusion emulates parts of a scene where not much light can get to.
	For example, the concave corners and edges of an object will be darker.

	There are a few way to approach SSAO, but my Vuk implementation uses the two thin g-buffers rendered in GBufferPass (position + normal).
*/

namespace vuk {
class PerThreadContext;
}

class SSAOPass : public GraphicsPass {
  public:
	static constexpr u16 KERNEL_SIZE = 64;

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt, struct UniformStore& uniforms, class PipelineStore& ps) override;
	void prep(vuk::PerThreadContext& ptc, struct Context& ctxt, struct RenderInfo& info) override;
	void render(vuk::PerThreadContext& ptc, struct Context& ctxt, vuk::RenderGraph& rg, const class SceneRenderer& renderer, struct RenderInfo& info) override;

  private:
	u32 m_width, m_height;
	vuk::Texture m_random_normal;
	std::array<glm::vec3, KERNEL_SIZE> m_kernel;
};
