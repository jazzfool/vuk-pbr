#include "Atmosphere.hpp"

#include "../Context.hpp"
#include "../Resource.hpp"
#include "../GfxUtil.hpp"
#include "../Mesh.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/CommandBuffer.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

void AtmosphericSkyCubemap::setup(Context& ctxt) {
	vuk::PipelineBaseCreateInfo sky;
	sky.add_shader(get_resource_string("Resources/Shaders/sky.vert"), "sky.vert");
	sky.add_shader(get_resource_string("Resources/Shaders/sky.frag"), "sky.frag");
	ctxt.vuk_context->create_named_pipeline("sky", sky);

	vuk::PipelineBaseCreateInfo skybox;
	skybox.add_shader(get_resource_string("Resources/Shaders/skybox.vert"), "skybox.vert");
	skybox.add_shader(get_resource_string("Resources/Shaders/skybox.frag"), "skybox.frag");
	ctxt.vuk_context->create_named_pipeline("skybox", skybox);
}

glm::mat4 AtmosphericSkyCubemap::skybox_model_matrix(const Perspective& cam_proj, glm::vec3 cam_pos) {
	return TransformComponent{}.translate(cam_pos).scale({cam_proj.far / sqrt(3), cam_proj.far / sqrt(3), cam_proj.far / sqrt(3)}).matrix;
}

void AtmosphericSkyCubemap::init(vuk::PerThreadContext& ptc, Context& ctxt, const RenderMesh& cube, glm::vec3 light_direction) {
	m_cubemap = gfx_util::alloc_cubemap(2048, 2048, ptc);

	auto equirectangular = ctxt.vuk_context->allocate_texture({
		.imageType = vuk::ImageType::e2D,
		.format = vuk::Format::eR32G32B32A32Sfloat,
		.extent = vuk::Extent3D{4096, 2048, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vuk::Samples::e1,
		.tiling = vuk::ImageTiling::eOptimal,
		.usage = vuk::ImageUsageFlagBits::eColorAttachment | vuk::ImageUsageFlagBits::eSampled,
		.sharingMode = vuk::SharingMode::eExclusive,
	});

	{
		vuk::RenderGraph rg;

		rg.add_pass({.resources = {"equirectangular"_image(vuk::eColorWrite)}, .execute = [light_direction](vuk::CommandBuffer& cbuf) {
						 cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
							 .set_scissor(0, vuk::Rect2D::framebuffer())
							 .bind_graphics_pipeline("sky")
							 .push_constants(vuk::ShaderStageFlagBits::eFragment, 0, light_direction)
							 .draw(3, 1, 0, 0);
					 }});

		rg.attach_image("equirectangular",
						vuk::ImageAttachment{
							.image = *equirectangular.image,
							.image_view = *equirectangular.view,
							.extent = vuk::Extent2D{4096, 2048},
							.format = vuk::Format::eR32G32B32A32Sfloat,
							.sample_count = vuk::Samples::e1,
							.clear_value = vuk::ClearColor{0.f, 0.f, 0.f, 1.f},
						},
						vuk::Access::eClear, vuk::eFragmentSampled);

		auto erg = std::move(rg).link(ptc);
		vuk::execute_submit_and_wait(ptc, std::move(erg));
	}

	const glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const glm::mat4 capture_views[] = {glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

	vuk::ImageViewCreateInfo ivci{.image = *m_cubemap.image,
								  .viewType = vuk::ImageViewType::e2D,
								  .format = vuk::Format::eR32G32B32A32Sfloat,
								  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

	for (u8 i = 0; i < 6; ++i) {
		vuk::RenderGraph rg;

		rg.add_pass({.resources = {"cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
						 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 2048, 2048))
							 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 2048, 2048))
							 .bind_vertex_buffer(0, *cube.verts, 0,
												 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat},
															 vuk::Ignore{vuk::Format::eR32G32Sfloat}})
							 .bind_index_buffer(*cube.inds, vuk::IndexType::eUint32)
							 .bind_sampled_image(0, 2, equirectangular,
												 vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																		.minFilter = vuk::Filter::eLinear,
																		.mipmapMode = vuk::SamplerMipmapMode::eLinear,
																		.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																		.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																		.addressModeW = vuk::SamplerAddressMode::eClampToEdge})
							 .bind_graphics_pipeline("equirectangular_to_cubemap");
						 glm::mat4* projection = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 0);
						 *projection = capture_projection;
						 glm::mat4* view = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
						 *view = capture_views[i];
						 cbuf.draw_indexed(cube.mesh.second.size(), 1, 0, 0, 0);
					 }});

		ivci.subresourceRange.baseArrayLayer = i;
		auto iv = ptc.create_image_view(ivci);

		rg.attach_image("cubemap_face",
						vuk::ImageAttachment{
							.image = *m_cubemap.image,
							.image_view = *iv,
							.extent = vuk::Extent2D{2048, 2048},
							.format = vuk::Format::eR32G32B32A32Sfloat,
						},
						vuk::Access::eClear, vuk::Access::eFragmentSampled);

		auto erg = std::move(rg).link(ptc);
		vuk::execute_submit_and_wait(ptc, std::move(erg));
	}

	vuk::ImageViewCreateInfo cubemap_ivci;
	cubemap_ivci.image = *m_cubemap.image;
	cubemap_ivci.viewType = vuk::ImageViewType::eCube;
	cubemap_ivci.format = vuk::Format::eR32G32B32A32Sfloat;
	cubemap_ivci.subresourceRange.aspectMask = vuk::ImageAspectFlagBits::eColor;
	cubemap_ivci.subresourceRange.baseArrayLayer = 0;
	cubemap_ivci.subresourceRange.baseMipLevel = 0;
	cubemap_ivci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	cubemap_ivci.subresourceRange.levelCount = 1;

	m_cubemap_view = ptc.create_image_view(cubemap_ivci);
}

void AtmosphericSkyCubemap::draw(vuk::CommandBuffer& cbuf, const vuk::Buffer& ubo, const RenderMesh& cube) {
	cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
		.set_scissor(0, vuk::Rect2D::framebuffer())
		.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
		.bind_vertex_buffer(0, *cube.verts, 0,
							vuk::Packed{
								vuk::Format::eR32G32B32Sfloat,
								vuk::Format::eR32G32B32Sfloat,
								vuk::Format::eR32G32Sfloat,
							})
		.bind_index_buffer(*cube.inds, vuk::IndexType::eUint32)
		.bind_graphics_pipeline("skybox")
		.bind_uniform_buffer(0, 0, ubo)
		.bind_sampled_image(0, 2, *m_cubemap_view, {});

	auto* model = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
	*model = skybox_model_matrix(cam_proj, cam_pos);
	cbuf.draw_indexed(cube.mesh.second.size(), 1, 0, 0, 0);
}
