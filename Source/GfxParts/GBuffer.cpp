#include "GBuffer.hpp"

#include "Atmosphere.hpp"
#include "../Context.hpp"
#include "../Scene.hpp"
#include "../Resource.hpp"
#include "../Renderer.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/CommandBuffer.hpp>

void GBufferPass::setup(Context& ctxt) {
	vuk::PipelineBaseCreateInfo pipeline;
	pipeline.add_shader(get_resource_string("Resources/Shaders/gbuffer.vert"), "gbuffer.vert");
	pipeline.add_shader(get_resource_string("Resources/Shaders/gbuffer.frag"), "gbuffer.frag");
	ctxt.vuk_context->create_named_pipeline("gbuffer", pipeline);
}

void GBufferPass::debug_position(vuk::CommandBuffer& cbuf) {
	cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
		.set_scissor(0, vuk::Rect2D::framebuffer())
		.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
		.bind_graphics_pipeline("debug")
		.bind_sampled_image(0, 0, "g_position", {})
		.draw(3, 1, 0, 0);
}

void GBufferPass::debug_normal(vuk::CommandBuffer& cbuf) {
	cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
		.set_scissor(0, vuk::Rect2D::framebuffer())
		.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
		.bind_graphics_pipeline("debug")
		.bind_sampled_image(0, 0, "g_normal", {})
		.draw(3, 1, 0, 0);
}

void GBufferPass::build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg, const SceneRenderer& renderer, const RenderInfo& info) {
	struct Uniforms {
		glm::mat4 proj;
		glm::mat4 view;
	} uniforms{info.cam_proj.matrix(), info.cam_view};

	auto [bubo, stub] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&uniforms, 1});
	auto ubo = bubo;

	ptc.wait_all_transfers();

	const auto skybox_mat = AtmosphericSkyCubemap::skybox_model_matrix(info.cam_proj, info.cam_pos);
	auto [bskybox_ubo, stbu] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&skybox_mat, 1});
	m_skybox_buffer = bskybox_ubo;

	auto pass = vuk::Pass{.resources = {"g_position"_image(vuk::eColorWrite), "g_normal"_image(vuk::eColorWrite), "depth_prepass"_image(vuk::eDepthStencilRW)},
						  .execute = [this, &renderer, ubo](vuk::CommandBuffer& cbuf) {
							  cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
								  .set_scissor(0, vuk::Rect2D::framebuffer())
								  .set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
								  .bind_graphics_pipeline("gbuffer")
								  .bind_uniform_buffer(0, 0, ubo);

							  { // render skybox
								  const auto& cube = renderer.scene().meshes.get(MeshCache::view("Cube"));
								  cbuf.bind_vertex_buffer(0, *cube.verts, 0,
														  vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
									  .bind_index_buffer(*cube.inds, vuk::IndexType::eUint32)
									  .bind_uniform_buffer(1, 0, m_skybox_buffer)
									  .bind_sampled_image(1, 1, renderer.scene().textures.get(TextureCache::view("Normal.Flat")), {})
									  .draw_indexed(cube.mesh.second.size(), 1, 0, 0, 0);
							  }

							  renderer.render(cbuf, [&](const MeshComponent& mesh, const vuk::Buffer& transform) {
								  cbuf.bind_sampled_image(1, 1, renderer.scene().textures.get(mesh.material.normal), {}).bind_uniform_buffer(1, 0, transform);
								  return vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat};
							  });
						  }};

	rg.add_pass(pass);

	rg.attach_managed("g_position", vuk::Format::eR16G16B16A16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("g_normal", vuk::Format::eR16G16B16A16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("depth_prepass", vuk::Format::eD32Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1, vuk::ClearDepthStencil{1.f, 0});
}
