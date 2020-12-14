#pragma once

#include "../Types.hpp"

#include <vuk/Image.hpp>

namespace vuk {
class PerThreadContext;
class RenderGraph;
} // namespace vuk

class AtmosphereRenderPass {
  public:
	static void setup(struct Context& ctxt);

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt);

	void build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg);

  private:
	vuk::Texture m_cubemap;
}; // namespace Atmosphere
