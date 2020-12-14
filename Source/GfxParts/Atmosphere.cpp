#include "Atmosphere.hpp"

#include "../Context.hpp"
#include "../Resource.hpp"

#include <vuk/RenderGraph.hpp>

void AtmosphereRenderPass::setup(Context& ctxt) {
	vuk::PipelineBaseCreateInfo pipeline;
	pipeline.add_shader(get_resource_string("Resources/Shaders/sky.vert"), "sky.vert");
	pipeline.add_shader(get_resource_string("Resources/Shaders/sky.frag"), "sky.frag");
	ctxt.vuk_context->create_named_pipeline("sky", pipeline);
}

void AtmosphereRenderPass::init(vuk::PerThreadContext& ptc, Context& ctxt) {
	vuk::RenderGraph rg;

	rg.add_pass({.resources = {"equirectangular"_image(vuk::eColorWrite)}, .execute = [](vuk::CommandBuffer& cbuf) {

				 }});

	rg.attach_managed("equirectangular", vuk::Format::eR8G8B8Srgb, vuk::Dimension2D::absolute(2048, 1024), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});

	auto erg = std::move(rg).link(ptc);
	vuk::execute_submit_and_wait(ptc, std::move(erg));
}
