#include "Renderer.hpp"

#include "Context.hpp"
#include "Resource.hpp"
#include "GfxUtil.hpp"
#include "Frustum.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/Pipeline.hpp>
#include <vuk/CommandBuffer.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/common.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

static const glm::vec3 LIGHT_DIRECTION = glm::normalize(glm::vec3(0, -2, 1));

void Renderer::init(Context& ctxt) {
	// initialize simple members

	m_ctxt = &ctxt;

	m_last_x = -1.f;
	m_last_y = -1.f;
	m_pitch = 0.f;
	m_yaw = 0.f;

	m_cam_pos = glm::vec3(0, 0, 3);
	m_cam_front = glm::vec3(0, 0, -3);
	m_cam_up = glm::vec3(0, 1, 0);

	m_sphere = load_obj("Resources/Meshes/Pillars.obj");
	m_cube = generate_cube();
	m_quad = generate_quad();

	// create the pipelines that are going to be used later

	vuk::PipelineBaseCreateInfo pbr;
	pbr.add_shader(get_resource_string("Resources/Shaders/pbr.vert"), "pbr.vert");
	pbr.add_shader(get_resource_string("Resources/Shaders/pbr.frag"), "pbr.frag");
	ctxt.vuk_context->create_named_pipeline("pbr", pbr);

	vuk::PipelineBaseCreateInfo equirectangular_to_cubemap;
	equirectangular_to_cubemap.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
	equirectangular_to_cubemap.add_shader(get_resource_string("Resources/Shaders/equirectangular_to_cubemap.frag"), "equirectangular_to_cubemap.frag");
	ctxt.vuk_context->create_named_pipeline("equirectangular_to_cubemap", equirectangular_to_cubemap);

	vuk::PipelineBaseCreateInfo irradiance;
	irradiance.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
	irradiance.add_shader(get_resource_string("Resources/Shaders/irradiance_convolution.frag"), "irradiance_convolution.frag");
	ctxt.vuk_context->create_named_pipeline("irradiance", irradiance);

	vuk::PipelineBaseCreateInfo prefilter;
	prefilter.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
	prefilter.add_shader(get_resource_string("Resources/Shaders/prefilter.frag"), "prefilter.frag");
	ctxt.vuk_context->create_named_pipeline("prefilter", prefilter);

	vuk::PipelineBaseCreateInfo brdf;
	brdf.add_shader(get_resource_string("Resources/Shaders/brdf.vert"), "brdf.vert");
	brdf.add_shader(get_resource_string("Resources/Shaders/brdf.frag"), "brdf.frag");
	ctxt.vuk_context->create_named_pipeline("brdf", brdf);

	vuk::PipelineBaseCreateInfo debug;
	debug.add_shader(get_resource_string("Resources/Shaders/debug.vert"), "debug.vert");
	debug.add_shader(get_resource_string("Resources/Shaders/debug.frag"), "debug.frag");
	ctxt.vuk_context->create_named_pipeline("debug", debug);

	vuk::PipelineBaseCreateInfo composite;
	composite.add_shader(get_resource_string("Resources/Shaders/composite.vert"), "composite.vert");
	composite.add_shader(get_resource_string("Resources/Shaders/composite.frag"), "composite.frag");
	ctxt.vuk_context->create_named_pipeline("composite", composite);

	auto ifc = ctxt.vuk_context->begin();
	auto ptc = ifc.begin();

	RenderMesh sphere_rm;
	sphere_rm.mesh = m_sphere;
	sphere_rm.upload(ptc);
	m_scene.meshes.insert("Sphere", std::move(sphere_rm));

	RenderMesh cube_rm;
	cube_rm.mesh = m_cube;
	cube_rm.upload(ptc);
	m_scene.meshes.insert("Cube", std::move(cube_rm));

	RenderMesh quad_rm;
	quad_rm.mesh = m_quad;
	quad_rm.upload(ptc);
	m_scene.meshes.insert("Quad", std::move(quad_rm));

	CascadedShadowRenderPass::setup(ctxt);
	SSAODepthPass::setup(ctxt);
	GBufferPass::setup(ctxt);
	VolumetricLightPass::setup(ctxt);
	AtmosphericSkyCubemap::setup(ctxt);

	m_cascaded_shadows.init(ptc, ctxt);
	m_ssao.init(ptc);
	m_volumetric_light.init(ctxt, ptc);
	m_atmosphere.init(ptc, ctxt, m_scene.meshes.get(MeshCache::view("Cube")), LIGHT_DIRECTION);

	// allocate a large buffer to dump all the model matrices in; this will be used a dynamic UBO for drawing by offsetting into it

	m_transform_buffer_alignment = std::max(ctxt.vkb_physical_device.properties.limits.minUniformBufferOffsetAlignment, sizeof(glm::mat4));
	m_transform_buffer =
		ctxt.vuk_context->allocate_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eUniformBuffer | vuk::BufferUsageFlagBits::eTransferDst,
										  std::pow(2, 18), m_transform_buffer_alignment);

	// load the textures that are going to be used later

	m_scene.textures.insert("Iron.Albedo", gfx_util::load_mipmapped_texture("Resources/Textures/rust_albedo.jpg", ptc));
	m_scene.textures.insert("Iron.Metallic", gfx_util::load_mipmapped_texture("Resources/Textures/rust_metallic.png", ptc));
	m_scene.textures.insert("Iron.Normal", gfx_util::load_mipmapped_texture("Resources/Textures/rust_normal.jpg", ptc, false));
	m_scene.textures.insert("Iron.Roughness", gfx_util::load_mipmapped_texture("Resources/Textures/rust_roughness.jpg", ptc));
	m_scene.textures.insert("Iron.AO", gfx_util::load_mipmapped_texture("Resources/Textures/rust_ao.jpg", ptc));

	m_scene.textures.insert("Normal.Flat", gfx_util::load_texture("Resources/Textures/flat_normal.png", ptc, false));

	m_scene_renderer = SceneRenderer::create(ctxt, m_scene);

	m_hdr_texture = gfx_util::load_cubemap_texture("Resources/Textures/forest_slope_1k.hdr", ptc);

	// m_hdr_texture is a 2:1 equirectangular; it needs to be converted to a cubemap

	m_env_cubemap = std::make_pair(gfx_util::alloc_cubemap(512, 512, ptc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																								  .minFilter = vuk::Filter::eLinear,
																								  .mipmapMode = vuk::SamplerMipmapMode::eLinear,
																								  .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																								  .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																								  .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
																								  .minLod = 0.f,
																								  .maxLod = 0.f});

	const glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	const glm::mat4 capture_views[] = {glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
									   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

	// upload the cube mesh

	auto [bverts, stub1] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span{m_cube.first});
	auto verts = std::move(bverts);
	auto [binds, stub2] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span{m_cube.second});
	auto inds = std::move(binds);

	ptc.wait_all_transfers();

	{
		vuk::ImageViewCreateInfo ivci{.image = *m_env_cubemap.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR32G32B32A32Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

		for (u32 i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"env_cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat},
																 vuk::Ignore{vuk::Format::eR32G32Sfloat}})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, m_hdr_texture,
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
							 cbuf.draw_indexed(m_cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			rg.attach_image("env_cubemap_face",
							vuk::ImageAttachment{
								.image = *m_env_cubemap.first.image,
								.image_view = *iv,
								.extent = vuk::Extent2D{512, 512},
								.format = vuk::Format::eR32G32B32A32Sfloat,
							},
							vuk::Access::eNone, vuk::Access::eFragmentSampled);

			auto erg = std::move(rg).link(ptc);
			vuk::execute_submit_and_wait(ptc, std::move(erg));
		}

		m_env_cubemap_iv = ptc.create_image_view(
			vuk::ImageViewCreateInfo{.image = *m_env_cubemap.first.image,
									 .viewType = vuk::ImageViewType::eCube,
									 .format = vuk::Format::eR32G32B32A32Sfloat,
									 .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 6}});
	}

	// filter this new cubemap to make an irradiance cubemap (for light emission)

	{
		m_irradiance_cubemap =
			std::make_pair(gfx_util::alloc_cubemap(32, 32, ptc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																						.minFilter = vuk::Filter::eLinear,
																						.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																						.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																						.addressModeW = vuk::SamplerAddressMode::eClampToEdge});

		vuk::ImageViewCreateInfo ivci{.image = *m_irradiance_cubemap.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR32G32B32A32Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

		for (unsigned i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"irradiance_cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 32, 32))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 32, 32))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat},
																 vuk::Ignore{vuk::Format::eR32G32Sfloat}})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, *m_env_cubemap_iv, m_env_cubemap.second)
								 .bind_graphics_pipeline("irradiance");
							 glm::mat4* projection = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 0);
							 *projection = capture_projection;
							 glm::mat4* view = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
							 *view = capture_views[i];
							 cbuf.draw_indexed(m_cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			vuk::ImageAttachment attachment;
			attachment.image = *m_irradiance_cubemap.first.image;
			attachment.image_view = *iv;
			attachment.extent = vuk::Extent2D{32, 32};
			attachment.format = vuk::Format::eR32G32B32A32Sfloat;

			rg.attach_image("irradiance_cubemap_face", attachment, vuk::Access::eNone, vuk::Access::eFragmentSampled);

			auto erg = std::move(rg).link(ptc);
			vuk::execute_submit_and_wait(ptc, std::move(erg));
		}

		vuk::ImageSubresourceRange img_subres_range;
		img_subres_range.aspectMask = vuk::ImageAspectFlagBits::eColor;
		img_subres_range.layerCount = 6;

		vuk::ImageViewCreateInfo ivci_irradiance;
		ivci_irradiance.image = *m_irradiance_cubemap.first.image;
		ivci_irradiance.viewType = vuk::ImageViewType::eCube;
		ivci_irradiance.format = vuk::Format::eR32G32B32A32Sfloat;
		ivci_irradiance.subresourceRange = img_subres_range;

		m_irradiance_cubemap_iv = ptc.create_image_view(ivci_irradiance);
	}

	// filter it again to create a prefiltered cubemap (for specular reflections at varying roughness levels)

	{
		vuk::SamplerCreateInfo sci;
		sci.magFilter = vuk::Filter::eLinear;
		sci.minFilter = vuk::Filter::eLinear;
		sci.mipmapMode = vuk::SamplerMipmapMode::eLinear;
		sci.addressModeU = vuk::SamplerAddressMode::eClampToEdge;
		sci.addressModeV = vuk::SamplerAddressMode::eClampToEdge;
		sci.addressModeW = vuk::SamplerAddressMode::eClampToEdge;
		sci.minLod = 0.f;
		sci.maxLod = 4.f;

		m_prefilter_cubemap = std::make_pair(gfx_util::alloc_cubemap(128, 128, ptc, 5), sci);

		vuk::ImageViewCreateInfo ivci{.image = *m_prefilter_cubemap.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR32G32B32A32Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

		u32 max_mip = 5;
		for (u32 mip = 0; mip < max_mip; ++mip) {
			u32 mip_wh = 128 * pow(0.5, mip);
			f32 roughness = (f32)mip / (f32)(max_mip - 1);

			ivci.subresourceRange.baseMipLevel = mip;

			for (unsigned i = 0; i < 6; ++i) {
				vuk::RenderGraph rg;
				rg.add_pass({.resources = {"prefilter_cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
								 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, (f32)mip_wh, (f32)mip_wh))
									 .set_scissor(0, vuk::Rect2D::absolute(0, 0, mip_wh, mip_wh))
									 .bind_vertex_buffer(0, verts, 0,
														 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat},
																	 vuk::Ignore{vuk::Format::eR32G32Sfloat}})
									 .bind_index_buffer(inds, vuk::IndexType::eUint32)
									 .bind_sampled_image(0, 2, *m_env_cubemap_iv,
														 vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																				.minFilter = vuk::Filter::eLinear,
																				.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																				.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																				.addressModeW = vuk::SamplerAddressMode::eClampToEdge})
									 .bind_graphics_pipeline("prefilter");
								 glm::mat4* projection = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 0);
								 *projection = capture_projection;
								 glm::mat4* view = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
								 *view = capture_views[i];
								 float* r = cbuf.map_scratch_uniform_binding<float>(0, 3);
								 *r = roughness;
								 cbuf.draw_indexed(m_cube.second.size(), 1, 0, 0, 0);
							 }});

				ivci.subresourceRange.baseArrayLayer = i;
				auto iv = ptc.create_image_view(ivci);

				vuk::ImageAttachment attachment;
				attachment.image = *m_prefilter_cubemap.first.image;
				attachment.image_view = *iv;
				attachment.extent = vuk::Extent2D{mip_wh, mip_wh};
				attachment.format = vuk::Format::eR32G32B32A32Sfloat;

				rg.attach_image("prefilter_cubemap_face", attachment, vuk::Access::eNone, vuk::Access::eFragmentSampled);

				auto erg = std::move(rg).link(ptc);
				vuk::execute_submit_and_wait(ptc, std::move(erg));
			}
		}

		m_prefilter_cubemap_iv = ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *m_prefilter_cubemap.first.image,
			.viewType = vuk::ImageViewType::eCube,
			.format = vuk::Format::eR32G32B32A32Sfloat,
			.subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .levelCount = 4, .layerCount = 6}});
	}

	// integrate BRDF into a LUT

	{
		m_brdf_lut = std::make_pair(gfx_util::alloc_lut(512, 512, ptc), vuk::SamplerCreateInfo{
																			.magFilter = vuk::Filter::eLinear,
																			.minFilter = vuk::Filter::eLinear,
																			.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																			.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																			.addressModeW = vuk::SamplerAddressMode::eClampToEdge,
																		});

		vuk::ImageViewCreateInfo ivci{.image = *m_brdf_lut.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR16G16Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor}};

		// upload the quad mesh

		auto [bqverts, _s1] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span{m_quad.first});
		auto qverts = std::move(bqverts);
		auto [qbinds, _s2] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span{m_quad.second});
		auto qinds = std::move(qbinds);

		ptc.wait_all_transfers();

		{
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"brdf_out"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(
									 0, qverts, 0,
									 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32B32Sfloat}, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(qinds, vuk::IndexType::eUint32)
								 .bind_graphics_pipeline("brdf");
							 cbuf.draw_indexed(m_quad.second.size(), 1, 0, 0, 0);
						 }});

			auto iv = ptc.create_image_view(ivci);

			rg.attach_image("brdf_out",
							vuk::ImageAttachment{
								.image = *m_brdf_lut.first.image,
								.image_view = *iv,
								.extent = vuk::Extent2D{512, 512},
								.format = vuk::Format::eR16G16Sfloat,
							},
							vuk::Access::eNone, vuk::Access::eFragmentSampled);

			auto erg = std::move(rg).link(ptc);
			vuk::execute_submit_and_wait(ptc, std::move(erg));
		}
	}

	auto entity = m_scene.registry.create();
	m_scene.registry.emplace<MeshComponent>(entity, MeshCache::view("Sphere"),
											Material{.albedo = TextureCache::view("Iron.Albedo"),
													 .metallic = TextureCache::view("Iron.Metallic"),
													 .roughness = TextureCache::view("Iron.Roughness"),
													 .normal = TextureCache::view("Iron.Normal"),
													 .ao = TextureCache::view("Iron.AO")});
	m_scene.registry.emplace<TransformComponent>(entity, TransformComponent{});

	auto entity2 = m_scene.registry.create();
	m_scene.registry.emplace<MeshComponent>(entity2, MeshCache::view("Quad"),
											Material{.albedo = TextureCache::view("Iron.Albedo"),
													 .metallic = TextureCache::view("Iron.Metallic"),
													 .roughness = TextureCache::view("Iron.Roughness"),
													 .normal = TextureCache::view("Iron.Normal"),
													 .ao = TextureCache::view("Iron.AO")});
	m_scene.registry.emplace<TransformComponent>(
		entity2, TransformComponent{}.translate({0, -1, 0}).rotate(glm::eulerAngleXYZ(glm::radians(-90.f), 0.f, 0.f)).scale({3, 3, 3}));
}

void Renderer::update() {
	constexpr static f32 cam_speed = 0.01f;

	const f32 dz = (glfwGetKey(m_ctxt->window, GLFW_KEY_W) | glfwGetKey(m_ctxt->window, GLFW_KEY_UP) - glfwGetKey(m_ctxt->window, GLFW_KEY_S) |
					glfwGetKey(m_ctxt->window, GLFW_KEY_DOWN)) *
				   cam_speed;
	const f32 dx = (glfwGetKey(m_ctxt->window, GLFW_KEY_D) | glfwGetKey(m_ctxt->window, GLFW_KEY_RIGHT) - glfwGetKey(m_ctxt->window, GLFW_KEY_A) |
					glfwGetKey(m_ctxt->window, GLFW_KEY_LEFT)) *
				   cam_speed;

	m_cam_pos += m_cam_front * dz;
	m_cam_pos += glm::normalize(glm::cross(m_cam_front, m_cam_up)) * dx;
}

void Renderer::render() {
	auto ifc = m_ctxt->vuk_context->begin();
	auto ptc = ifc.begin();

	auto rg = render_graph(ptc);
	rg.attach_swapchain("pbr_final", m_ctxt->vuk_swapchain, vuk::ClearColor{0.01f, 0.01f, 0.01f, 1.f});
	auto erg = std::move(rg).link(ptc);
	vuk::execute_submit_and_present_to_one(ptc, std::move(erg), m_ctxt->vuk_swapchain);
}

vuk::RenderGraph Renderer::render_graph(vuk::PerThreadContext& ptc) {
	struct Uniforms {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 light_space_matrix;
	};

	struct Cascades {
		float cascade_splits[CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT];
		glm::mat4 cascade_view_proj_mats[CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT];
		glm::vec3 light_direction;
	};

	auto meshes_view = m_scene.registry.view<MeshComponent, TransformComponent>();

	Perspective cam_perspective;
	cam_perspective.fovy = glm::radians(60.f);
	cam_perspective.aspect_ratio = static_cast<f32>(m_ctxt->vkb_swapchain.extent.width) / static_cast<f32>(m_ctxt->vkb_swapchain.extent.height);
	cam_perspective.near = 0.1f;
	cam_perspective.far = 100.f;

	const glm::mat4 cam_view = glm::lookAt(m_cam_pos, m_cam_pos + m_cam_front, m_cam_up);

	RenderInfo render_info;
	render_info.light_direction = LIGHT_DIRECTION;
	render_info.cam_proj = cam_perspective;
	render_info.cam_view = cam_view;
	render_info.cam_pos = m_cam_pos;
	render_info.cam_forward = m_cam_front;
	render_info.cam_up = m_cam_up;
	render_info.shadow_map = m_cascaded_shadows.shadow_map_view();

	Uniforms uniforms;

	uniforms.projection = cam_perspective.matrix();
	uniforms.view = cam_view;

	auto [bubo, stub3] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span(&uniforms, 1));
	auto ubo = bubo;

	Cascades cascades;

	cascades.light_direction = LIGHT_DIRECTION;

	render_info.cascades = m_cascaded_shadows.compute_cascades(render_info);

	for (u32 i = 0; i < render_info.cascades.size(); ++i) {
		cascades.cascade_splits[i] = render_info.cascades[i].split_depth;
		cascades.cascade_view_proj_mats[i] = render_info.cascades[i].view_proj_mat;
	}

	auto [bcascade_ubo, cascadestub] =
		ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{&cascades, 1});
	auto cascade_ubo = bcascade_ubo;

	m_ssao.width = m_ctxt->vkb_swapchain.extent.width;
	m_ssao.height = m_ctxt->vkb_swapchain.extent.height;

	m_gbuffer.width = m_ctxt->vkb_swapchain.extent.width;
	m_gbuffer.height = m_ctxt->vkb_swapchain.extent.height;

	m_volumetric_light.width = m_ctxt->vkb_swapchain.extent.width;
	m_volumetric_light.height = m_ctxt->vkb_swapchain.extent.height;

	m_atmosphere.cam_proj = cam_perspective;
	m_atmosphere.cam_pos = m_cam_pos;

	u32 offset = 0;
	meshes_view.each([&](MeshComponent& mesh, TransformComponent& transform) {
		ptc.upload(m_transform_buffer.subrange(offset, sizeof(glm::mat4)), std::span{&transform.matrix, 1});
		offset += m_transform_buffer_alignment;
	});

	ptc.wait_all_transfers();

	m_scene_renderer.update(ptc, m_scene, meshes_view);

	vuk::RenderGraph rg;

	// cascaded depth only pass

	m_cascaded_shadows.build(ptc, rg, m_scene_renderer, render_info);

	// gbuffer pass

	m_gbuffer.build(ptc, rg, m_scene_renderer, render_info);

	// ssao pass

	m_ssao.build(ptc, rg, render_info);

	// volumetric light pass

	m_volumetric_light.build(ptc, rg, render_info);

	// color pass

	struct PushConstants {
		glm::vec3 cam_pos;
		f32 _pad;
		glm::vec2 screen_size;
	} push_consts{m_cam_pos, 0.f, glm::vec2{m_ctxt->vkb_swapchain.extent.width, m_ctxt->vkb_swapchain.extent.height}};

	const vuk::SamplerCreateInfo map_sampler{.magFilter = vuk::Filter::eLinear,
											 .minFilter = vuk::Filter::eLinear,
											 .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
											 .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
											 .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
											 .anisotropyEnable = VK_TRUE,
											 .maxAnisotropy = 16.f,
											 .minLod = 0.f,
											 .maxLod = 16.f};

	rg.add_pass({.resources =
					 {
						 "pbr_msaa"_image(vuk::eColorWrite),
						 "pbr_depth"_image(vuk::eDepthStencilRW),
						 "ssao_blurred"_image(vuk::eFragmentSampled),
					 },
				 .execute = [this, meshes_view, map_sampler, ubo, cascade_ubo, push_consts](vuk::CommandBuffer& cbuf) {
					 const auto sci = vuk::SamplerCreateInfo{.addressModeU = vuk::SamplerAddressMode::eClampToBorder,
															 .addressModeV = vuk::SamplerAddressMode::eClampToBorder,
															 .addressModeW = vuk::SamplerAddressMode::eClampToBorder};

					 // draw skybox
					 m_atmosphere.draw(cbuf, ubo, m_scene.meshes.get(MeshCache::view("Cube")));

					 cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
						 .set_scissor(0, vuk::Rect2D::framebuffer())
						 .set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
						 .bind_uniform_buffer(0, 0, ubo)
						 .push_constants(vuk::ShaderStageFlagBits::eFragment, 0, push_consts)
						 .bind_sampled_image(0, 1, *m_irradiance_cubemap_iv, m_irradiance_cubemap.second)
						 .bind_sampled_image(0, 2, *m_prefilter_cubemap_iv, m_prefilter_cubemap.second)
						 .bind_sampled_image(0, 3, m_brdf_lut.first, m_brdf_lut.second)
						 .bind_sampled_image(0, 4, m_cascaded_shadows.shadow_map_view(), sci)
						 .bind_sampled_image(0, 5, "ssao_blurred", sci)
						 .bind_uniform_buffer(0, 6, cascade_ubo)
						 .bind_graphics_pipeline("pbr");

					 u64 offset = 0;
					 meshes_view.each([&](MeshComponent& mesh_comp, TransformComponent& transform_comp) {
						 cbuf.bind_vertex_buffer(0, *m_scene.meshes.get(mesh_comp.mesh).verts, 0,
												 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
							 .bind_index_buffer(*m_scene.meshes.get(mesh_comp.mesh).inds, vuk::IndexType::eUint32)
							 .bind_sampled_image(2, 0, m_scene.textures.get(mesh_comp.material.albedo), map_sampler)
							 .bind_sampled_image(2, 1, m_scene.textures.get(mesh_comp.material.normal), map_sampler)
							 .bind_sampled_image(2, 2, m_scene.textures.get(mesh_comp.material.metallic), map_sampler)
							 .bind_sampled_image(2, 3, m_scene.textures.get(mesh_comp.material.roughness), map_sampler)
							 .bind_sampled_image(2, 4, m_scene.textures.get(mesh_comp.material.ao), map_sampler)
							 .bind_uniform_buffer(1, 0, m_transform_buffer.subrange(offset, m_transform_buffer_alignment));
						 cbuf.draw_indexed(m_scene.meshes.get(mesh_comp.mesh).mesh.second.size(), 1, 0, 0, 0);
						 offset += m_transform_buffer_alignment;
					 });
				 }});

	// composite pass

	rg.add_pass({.resources =
					 {
						 "pbr_composite"_image(vuk::eColorWrite),
						 "pbr_msaa"_image(vuk::eFragmentSampled),
						 "volumetric_light_blurred"_image(vuk::eFragmentSampled),
					 },
				 .execute = [](vuk::CommandBuffer& cbuf) {
					 const auto sci = vuk::SamplerCreateInfo{.addressModeU = vuk::SamplerAddressMode::eClampToBorder,
															 .addressModeV = vuk::SamplerAddressMode::eClampToBorder,
															 .addressModeW = vuk::SamplerAddressMode::eClampToBorder};

					 cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
						 .set_scissor(0, vuk::Rect2D::framebuffer())
						 .bind_graphics_pipeline("composite")
						 .bind_sampled_image(0, 0, "pbr_msaa", sci)
						 .bind_sampled_image(0, 1, "volumetric_light_blurred", sci)
						 .draw(3, 1, 0, 0);
				 }});

	rg.attach_managed("pbr_msaa", static_cast<vuk::Format>(m_ctxt->vkb_swapchain.image_format),
					  vuk::Dimension2D::absolute(m_ctxt->vkb_swapchain.extent.width, m_ctxt->vkb_swapchain.extent.height), vuk::Samples::e1,
					  vuk::ClearColor{0.01f, 0.01f, 0.01f, 1.f});
	rg.attach_managed("pbr_composite", static_cast<vuk::Format>(m_ctxt->vkb_swapchain.image_format),
					  vuk::Dimension2D::absolute(m_ctxt->vkb_swapchain.extent.width, m_ctxt->vkb_swapchain.extent.height), vuk::Samples::e8,
					  vuk::ClearColor{0.f, 0.f, 0.f, 1.f});
	rg.attach_managed("pbr_depth", vuk::Format::eD32Sfloat, vuk::Dimension2D::framebuffer(), vuk::Samples::Framebuffer{}, vuk::ClearDepthStencil{1.f, 0});
	rg.resolve_resource_into("pbr_final", "pbr_composite");

	return rg;
}

void Renderer::mouse_event(f64 x_pos, f64 y_pos) {
	static constexpr f32 sensitivity = 0.05f;

	if (m_last_x == -1.f) {
		m_last_x = x_pos;
		m_last_y = y_pos;
	}

	const f32 dx = (x_pos - m_last_x) * sensitivity;
	const f32 dy = (m_last_y - y_pos) * sensitivity;

	m_last_x = x_pos;
	m_last_y = y_pos;

	m_yaw += dx;
	m_pitch += dy;

	m_pitch = std::min(std::max(m_pitch, -89.f), 89.f);

	glm::vec3 dir;
	dir.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	dir.y = std::sin(glm::radians(m_pitch));
	dir.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	m_cam_front = glm::normalize(dir);
}
