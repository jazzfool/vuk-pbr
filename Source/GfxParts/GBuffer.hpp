#pragma once

#include "GraphicsPass.hpp"
#include "../Types.hpp"
#include "../Perspective.hpp"

#include <glm/mat4x4.hpp>
#include <vuk/Buffer.hpp>

/*
	This is a forward renderer, but some thin g-buffers are still needed for effects like SSAO.
*/

namespace vuk {
struct Pass;
class PerThreadContext;
struct RenderGraph;
class CommandBuffer;
} // namespace vuk

class GBufferPass : public GraphicsPass {
  public:
	void debug_position(vuk::CommandBuffer& cbuf);
	void debug_normal(vuk::CommandBuffer& cbuf);

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt, struct UniformStore& uniforms, class PipelineStore& ps) override;
	void prep(vuk::PerThreadContext& ptc, struct Context& ctxt, struct RenderInfo& info) override;
	void render(vuk::PerThreadContext& ptc, struct Context& ctxt, vuk::RenderGraph& rg, const class SceneRenderer& renderer, struct RenderInfo& info) override;

  private:
	u32 m_width, m_height;
};
