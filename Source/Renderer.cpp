#include "Renderer.hpp"

#include "Context.hpp"
#include "Resource.hpp"
#include "GfxUtil.hpp"

#include <vuk/RenderGraph.hpp>
#include <vuk/Pipeline.hpp>
#include <vuk/CommandBuffer.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

std::optional<Renderer> Renderer::create(Context& ctxt) {
	Renderer renderer;

	// initialize simple members

	renderer.m_ctxt = &ctxt;

	renderer.m_last_x = -1.f;
	renderer.m_last_y = -1.f;
	renderer.m_pitch = 0.f;
	renderer.m_yaw = 0.f;

	renderer.m_cam_pos = glm::vec3(0, 0, 3);
	renderer.m_cam_front = glm::vec3(0, 0, -3);
	renderer.m_cam_up = glm::vec3(0, 1, 0);

	renderer.m_sphere = load_obj("Resources/Meshes/Sphere.obj");
	renderer.m_cube = generate_cube();
	renderer.m_quad = generate_quad();

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

	auto ifc = ctxt.vuk_context->begin();
	auto ptc = ifc.begin();

	// allocate a large buffer to dump all the model matrices in; this will be used a dynamic UBO for drawing by offsetting into it

	renderer.m_transform_buffer = ctxt.vuk_context->allocate_buffer(
		vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eUniformBuffer | vuk::BufferUsageFlagBits::eTransferDst, std::pow(2, 18), sizeof(glm::mat4));

	// load the textures that are going to be used later

	renderer.m_scene.textures.insert("Iron.Albedo", gfx_util::load_mipmapped_texture("Resources/Textures/rust_albedo.jpg", ptc));
	renderer.m_scene.textures.insert("Iron.Metallic", gfx_util::load_mipmapped_texture("Resources/Textures/rust_metallic.png", ptc));
	renderer.m_scene.textures.insert("Iron.Normal", gfx_util::load_mipmapped_texture("Resources/Textures/rust_normal.jpg", ptc));
	renderer.m_scene.textures.insert("Iron.Roughness", gfx_util::load_mipmapped_texture("Resources/Textures/rust_roughness.jpg", ptc));
	renderer.m_scene.textures.insert("Iron.AO", gfx_util::load_mipmapped_texture("Resources/Textures/rust_ao.jpg", ptc));

	RenderMesh sphere_rm;
	sphere_rm.mesh = renderer.m_sphere;
	sphere_rm.upload(ptc);
	renderer.m_scene.meshes.insert("Sphere", std::move(sphere_rm));

	RenderMesh cube_rm;
	cube_rm.mesh = renderer.m_cube;
	cube_rm.upload(ptc);
	renderer.m_scene.meshes.insert("Cube", std::move(cube_rm));

	renderer.m_hdr_texture = gfx_util::load_cubemap_texture("Resources/Textures/forest_slope_1k.hdr", ptc);

	// m_hdr_texture is a 2:1 equirectangular; it needs to be converted to a cubemap

	renderer.m_env_cubemap =
		std::make_pair(gfx_util::alloc_cubemap(512, 512, ptc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
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

	auto [bverts, stub1] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer,
													 std::span(&renderer.m_cube.first[0], renderer.m_cube.first.size()));
	auto verts = std::move(bverts);
	auto [binds, stub2] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer,
													std::span(&renderer.m_cube.second[0], renderer.m_cube.second.size()));
	auto inds = std::move(binds);

	ptc.wait_all_transfers();

	{
		vuk::ImageViewCreateInfo ivci{.image = *renderer.m_env_cubemap.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR32G32B32A32Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

		for (u32 i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"env_cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, renderer.m_hdr_texture,
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
							 cbuf.draw_indexed(renderer.m_cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			rg.attach_image("env_cubemap_face",
							vuk::ImageAttachment{
								.image = *renderer.m_env_cubemap.first.image,
								.image_view = *iv,
								.extent = vuk::Extent2D{512, 512},
								.format = vuk::Format::eR32G32B32A32Sfloat,
							},
							vuk::Access::eNone, vuk::Access::eFragmentSampled);

			auto erg = std::move(rg).link(ptc);
			vuk::execute_submit_and_wait(ptc, std::move(erg));
		}

		renderer.m_env_cubemap_iv = ptc.create_image_view(
			vuk::ImageViewCreateInfo{.image = *renderer.m_env_cubemap.first.image,
									 .viewType = vuk::ImageViewType::eCube,
									 .format = vuk::Format::eR32G32B32A32Sfloat,
									 .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 6}});
	}

	// filter this new cubemap to make an irradiance cubemap (for light emission)

	{
		renderer.m_irradiance_cubemap =
			std::make_pair(gfx_util::alloc_cubemap(32, 32, ptc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																						.minFilter = vuk::Filter::eLinear,
																						.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																						.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																						.addressModeW = vuk::SamplerAddressMode::eClampToEdge});

		vuk::ImageViewCreateInfo ivci{.image = *renderer.m_irradiance_cubemap.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR32G32B32A32Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 1}};

		for (unsigned i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"irradiance_cubemap_face"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 32, 32))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 32, 32))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, *renderer.m_env_cubemap_iv, renderer.m_env_cubemap.second)
								 .bind_graphics_pipeline("irradiance");
							 glm::mat4* projection = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 0);
							 *projection = capture_projection;
							 glm::mat4* view = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
							 *view = capture_views[i];
							 cbuf.draw_indexed(renderer.m_cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			vuk::ImageAttachment attachment;
			attachment.image = *renderer.m_irradiance_cubemap.first.image;
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
		ivci_irradiance.image = *renderer.m_irradiance_cubemap.first.image;
		ivci_irradiance.viewType = vuk::ImageViewType::eCube;
		ivci_irradiance.format = vuk::Format::eR32G32B32A32Sfloat;
		ivci_irradiance.subresourceRange = img_subres_range;

		renderer.m_irradiance_cubemap_iv = ptc.create_image_view(ivci_irradiance);
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

		renderer.m_prefilter_cubemap = std::make_pair(gfx_util::alloc_cubemap(128, 128, ptc, 5), sci);

		vuk::ImageViewCreateInfo ivci{.image = *renderer.m_prefilter_cubemap.first.image,
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
														 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
									 .bind_index_buffer(inds, vuk::IndexType::eUint32)
									 .bind_sampled_image(0, 2, *renderer.m_env_cubemap_iv,
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
								 cbuf.draw_indexed(renderer.m_cube.second.size(), 1, 0, 0, 0);
							 }});

				ivci.subresourceRange.baseArrayLayer = i;
				auto iv = ptc.create_image_view(ivci);

				vuk::ImageAttachment attachment;
				attachment.image = *renderer.m_prefilter_cubemap.first.image;
				attachment.image_view = *iv;
				attachment.extent = vuk::Extent2D{mip_wh, mip_wh};
				attachment.format = vuk::Format::eR32G32B32A32Sfloat;

				rg.attach_image("prefilter_cubemap_face", attachment, vuk::Access::eNone, vuk::Access::eFragmentSampled);

				auto erg = std::move(rg).link(ptc);
				vuk::execute_submit_and_wait(ptc, std::move(erg));
			}
		}

		renderer.m_prefilter_cubemap_iv = ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *renderer.m_prefilter_cubemap.first.image,
			.viewType = vuk::ImageViewType::eCube,
			.format = vuk::Format::eR32G32B32A32Sfloat,
			.subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .levelCount = 4, .layerCount = 6}});
	}

	// integrate BRDF into a LUT

	{
		renderer.m_brdf_lut = std::make_pair(gfx_util::alloc_lut(512, 512, ptc), vuk::SamplerCreateInfo{
																					 .magFilter = vuk::Filter::eLinear,
																					 .minFilter = vuk::Filter::eLinear,
																					 .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																					 .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																					 .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
																				 });

		vuk::ImageViewCreateInfo ivci{.image = *renderer.m_brdf_lut.first.image,
									  .viewType = vuk::ImageViewType::e2D,
									  .format = vuk::Format::eR16G16Sfloat,
									  .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor}};

		// upload the quad mesh

		auto [bqverts, _s1] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer,
														std::span(&renderer.m_quad.first[0], renderer.m_quad.first.size()));
		auto qverts = std::move(bqverts);
		auto [qbinds, _s2] = ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer,
													   std::span(&renderer.m_quad.second[0], renderer.m_quad.second.size()));
		auto qinds = std::move(qbinds);

		ptc.wait_all_transfers();

		{
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"brdf_out"_image(vuk::eColorWrite)}, .execute = [&](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Rect2D::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(0, qverts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(qinds, vuk::IndexType::eUint32)
								 .bind_graphics_pipeline("brdf");
							 cbuf.draw_indexed(renderer.m_quad.second.size(), 1, 0, 0, 0);
						 }});

			auto iv = ptc.create_image_view(ivci);

			rg.attach_image("brdf_out",
							vuk::ImageAttachment{
								.image = *renderer.m_brdf_lut.first.image,
								.image_view = *iv,
								.extent = vuk::Extent2D{512, 512},
								.format = vuk::Format::eR16G16Sfloat,
							},
							vuk::Access::eNone, vuk::Access::eFragmentSampled);

			auto erg = std::move(rg).link(ptc);
			vuk::execute_submit_and_wait(ptc, std::move(erg));
		}
	}

	auto entity = renderer.m_scene.registry.create();
	renderer.m_scene.registry.emplace<MeshComponent>(entity, MeshCache::view("Sphere"),
													 Material{.albedo = TextureCache::view("Iron.Albedo"),
															  .metallic = TextureCache::view("Iron.Metallic"),
															  .roughness = TextureCache::view("Iron.Roughness"),
															  .normal = TextureCache::view("Iron.Normal"),
															  .ao = TextureCache::view("Iron.AO")});
	renderer.m_scene.registry.emplace<TransformComponent>(entity, TransformComponent{}.rotate(glm::angleAxis(glm::degrees(90.f), glm::vec3(1, 0, 0))));

	auto entity2 = renderer.m_scene.registry.create();
	renderer.m_scene.registry.emplace<MeshComponent>(entity2, MeshCache::view("Cube"),
													 Material{.albedo = TextureCache::view("Iron.Albedo"),
															  .metallic = TextureCache::view("Iron.Metallic"),
															  .roughness = TextureCache::view("Iron.Roughness"),
															  .normal = TextureCache::view("Iron.Normal"),
															  .ao = TextureCache::view("Iron.AO")});
	renderer.m_scene.registry.emplace<TransformComponent>(
		entity2, TransformComponent{}.translate({10, 5, 5}).rotate(glm::angleAxis(glm::degrees(90.f), glm::vec3(1, 0, 0))));

	return renderer;
}

void Renderer::update() {
	constexpr static f32 cam_speed = 0.0025f;

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
	};

	Uniforms uniforms;

	uniforms.projection = glm::perspective(glm::degrees(60.f), 800.f / 600.f, 0.1f, 1000.f);
	uniforms.projection[1][1] *= -1.f; // flip the projection so it's the right way up
	uniforms.view = glm::lookAt(m_cam_pos, m_cam_pos + m_cam_front, m_cam_up);

	auto [bubo, stub3] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span(&uniforms, 1));
	auto ubo = bubo;

	auto meshes_view = m_scene.registry.view<MeshComponent, TransformComponent>();

	std::vector<glm::mat4> transforms;
	transforms.reserve(meshes_view.size_hint());
	meshes_view.each([&](MeshComponent& mesh, TransformComponent& transform) { transforms.push_back(transform.matrix); });
	ptc.upload(m_transform_buffer, std::span{transforms});

	ptc.wait_all_transfers();

	const vuk::SamplerCreateInfo map_sampler{.magFilter = vuk::Filter::eLinear,
											 .minFilter = vuk::Filter::eLinear,
											 .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
											 .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
											 .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
											 .anisotropyEnable = VK_TRUE,
											 .maxAnisotropy = 16.f,
											 .minLod = 0.f,
											 .maxLod = 16.f};

	vuk::RenderGraph rg;
	rg.add_pass({.resources = {"pbr_msaa"_image(vuk::eColorWrite), "pbr_depth"_image(vuk::eDepthStencilRW)},
				 .execute = [meshes_view, map_sampler, ubo, this](vuk::CommandBuffer& cbuf) {
					 cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
						 .set_scissor(0, vuk::Rect2D::framebuffer())
						 .set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
						 .push_constants(vuk::ShaderStageFlagBits::eFragment, 0, m_cam_pos)
						 .bind_sampled_image(0, 1, *m_irradiance_cubemap_iv, m_irradiance_cubemap.second)
						 .bind_sampled_image(0, 2, *m_prefilter_cubemap_iv, m_prefilter_cubemap.second)
						 .bind_sampled_image(0, 3, m_brdf_lut.first, m_brdf_lut.second)
						 .bind_graphics_pipeline("pbr")
						 .bind_uniform_buffer(0, 0, ubo);

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
							 .bind_uniform_buffer(1, 0, m_transform_buffer.subrange(offset, sizeof(glm::mat4)));
						 cbuf.draw_indexed(m_scene.meshes.get(mesh_comp.mesh).mesh.second.size(), 1, 0, 0, 0);
						 offset += sizeof(glm::mat4);
					 });
				 }});

	rg.attach_managed("pbr_msaa", vuk::Format::eR8G8B8A8Srgb, vuk::Dimension2D::framebuffer(), vuk::Samples::e8, vuk::ClearColor{0.01f, 0.01f, 0.01f, 1.f});
	rg.attach_managed("pbr_depth", vuk::Format::eD32Sfloat, vuk::Dimension2D::framebuffer(), vuk::Samples::Framebuffer{}, vuk::ClearDepthStencil{1.0f, 0});
	rg.resolve_resource_into("pbr_final", "pbr_msaa");

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