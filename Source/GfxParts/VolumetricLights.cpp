#include "VolumetricLights.hpp"

#include "../Context.hpp"
#include "../Resource.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/CommandBuffer.hpp>

void VolumetricLightPass::setup(Context& ctxt) {
	vuk::PipelineBaseCreateInfo volumetric_light;
	volumetric_light.add_shader(get_resource_string("Resources/Shaders/volumetric_light.vert"), "volumetric_light.vert");
	volumetric_light.add_shader(get_resource_string("Resources/Shaders/volumetric_light.frag"), "volumetric_light.frag");
	ctxt.vuk_context->create_named_pipeline("volumetric_light", volumetric_light);

	vuk::PipelineBaseCreateInfo blur;
	blur.add_shader(get_resource_string("Resources/Shaders/volumetric_light_blur.vert"), "volumetric_light_blur.vert");
	blur.add_shader(get_resource_string("Resources/Shaders/volumetric_light_blur.frag"), "volumetric_light_blur.frag");
	ctxt.vuk_context->create_named_pipeline("volumetric_light_blur", blur);
}

void VolumetricLightPass::debug(vuk::CommandBuffer& cbuf) {
	cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
		.set_scissor(0, vuk::Rect2D::framebuffer())
		.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
		.bind_graphics_pipeline("debug")
		.bind_sampled_image(0, 0, "volumetric_light_blurred", {})
		.draw(3, 1, 0, 0);
}

void VolumetricLightPass::init(Context& ctxt, vuk::PerThreadContext& ptc) {
	m_verts = ctxt.vuk_context->allocate_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer | vuk::BufferUsageFlagBits::eTransferDst, 4,
												sizeof(Vertex));
	m_inds = ctxt.vuk_context->allocate_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer | vuk::BufferUsageFlagBits::eTransferDst, 6,
											   sizeof(u32));

	const auto quad = generate_quad();

	ptc.upload(m_verts, std::span{quad.first});
	ptc.upload(m_inds, std::span{quad.second});

	ptc.wait_all_transfers();
}

void VolumetricLightPass::build(vuk::PerThreadContext& ptc, vuk::RenderGraph& rg) {
	struct Uniforms {
		f32 cascade_splits[CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT];
		glm::mat4 cascade_view_proj_mats[CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT];
		glm::mat4 inv_view;
		glm::vec3 light_direction;
		f32 _pad;
		glm::vec3 cam_pos;
	} uniforms;

	struct Camera {
		glm::mat4 inv_view_proj;
		glm::vec2 clip_range;
	} camera;

	uniforms.light_direction = light_direction;
	uniforms.cam_pos = cam_pos;

	for (u8 i = 0; i < CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT; ++i) {
		uniforms.cascade_splits[i] = cascades[i].split_depth;
		uniforms.cascade_view_proj_mats[i] = cascades[i].view_proj_mat;
	}

	uniforms.inv_view = glm::inverse(cam_view);

	camera.inv_view_proj = glm::inverse(cam_proj.matrix() * cam_view);
	camera.clip_range.x = cam_proj.near;
	camera.clip_range.y = cam_proj.far;

	auto [bubo, stub] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&uniforms, 1});
	auto ubo = bubo;

	auto [bcam, stub2] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&camera, 1});
	auto cam = bcam;

	ptc.wait_all_transfers();

	rg.add_pass(vuk::Pass{
		.resources = {"volumetric_light"_image(vuk::eColorWrite), "volumetric_depth"_image(vuk::eDepthStencilRW), "g_position"_image(vuk::eFragmentSampled)},
		.execute = [this, ubo, cam](vuk::CommandBuffer& cbuf) {
			cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, width, height))
				.set_scissor(0, vuk::Rect2D::absolute(0, 0, width, height))
				.bind_vertex_buffer(0, m_verts, 0,
									vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat}, vuk::Format::eR32G32Sfloat})
				.bind_index_buffer(m_inds, vuk::IndexType::eUint32)
				.bind_graphics_pipeline("volumetric_light")
				.bind_uniform_buffer(0, 0, cam)
				.bind_sampled_image(0, 1, "g_position", {})
				.bind_sampled_image(0, 2, shadow_map, {})
				.bind_uniform_buffer(0, 3, ubo)
				.draw_indexed(6, 1, 0, 0, 0);
		}});

	rg.add_pass(vuk::Pass{.resources = {"volumetric_light_blurred"_image(vuk::eColorWrite), "volumetric_light"_image(vuk::eFragmentSampled)},
						  .execute = [this](vuk::CommandBuffer& cbuf) {
							  cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
								  .set_scissor(0, vuk::Rect2D::absolute(0, 0, width, height))
								  .bind_graphics_pipeline("volumetric_light_blur")
								  .bind_sampled_image(0, 0, "volumetric_light", {})
								  .draw(3, 1, 0, 0);
						  }});

	rg.attach_managed("volumetric_light", vuk::Format::eR16G16B16A16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("volumetric_light_blurred", vuk::Format::eR16G16B16A16Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("volumetric_depth", vuk::Format::eD32Sfloat, vuk::Dimension2D::absolute(width, height), vuk::Samples::e1, vuk::ClearDepthStencil{1.f, 0});
}
