#pragma once

#include "../Types.hpp"

#include <glm/mat4x4.hpp>
#include <vuk/Image.hpp>

/*
	This is a forward renderer, but some thin g-buffers are still needed for effects like SSAO.
*/

namespace vuk {
struct Pass;
class PerThreadContext;
struct RenderGraph;
class CommandBuffer;
} // namespace vuk

class GBufferPass {
  public:
	static void setup(struct Context& ctxt);

	static void debug_position(vuk::CommandBuffer& cbuf);
	static void debug_normal(vuk::CommandBuffer& cbuf);

	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, const class SceneRenderer& renderer);

	glm::mat4 cam_proj;
	glm::mat4 cam_view;

	u32 width, height;

  private:
};
