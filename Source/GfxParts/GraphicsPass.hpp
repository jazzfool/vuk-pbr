#pragma once

namespace vuk {
class PerThreadContext;
struct RenderGraph;
} // namespace vuk

class GraphicsPass {
  public:
	virtual void init(vuk::PerThreadContext& ptc, struct Context& ctxt, struct UniformStore& uniforms, class PipelineStore& ps) = 0;
	virtual void prep(vuk::PerThreadContext& ptc, struct Context& ctxt, struct RenderInfo& info) = 0;
	virtual void render(
		vuk::PerThreadContext& ptc, struct Context& ctxt, vuk::RenderGraph& rg, const class SceneRenderer& renderer, struct RenderInfo& info) = 0;
};
