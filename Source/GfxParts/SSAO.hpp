#pragma once

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

class SSAODepthPass {
  public:
	static constexpr u16 KERNEL_SIZE = 64;

	static void setup(struct Context& ctxt);

	void init(vuk::PerThreadContext& ptc);
	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg);

	Perspective cam_proj;
	u32 width, height;

  private:
	vuk::Texture m_random_normal;
	std::array<glm::vec3, KERNEL_SIZE> m_kernel;
};
