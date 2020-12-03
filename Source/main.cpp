#define STB_IMAGE_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <stb_image/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vuk/CommandBuffer.hpp>
#include <vuk/Context.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Types.hpp>

#include "Resource.hpp"

#define M_PI 3.14159265358979323846

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

using Mesh = std::pair<std::vector<Vertex>, std::vector<unsigned>>;

Mesh generate_quad() {
	return Mesh(
		std::vector<Vertex>{
			Vertex{{-1, 1, 0}, {0, 0, 1}, {0, 1}},
			Vertex{{-1, -1, 0}, {0, 0, 1}, {0, 0}},
			Vertex{{1, 1, 0}, {0, 0, 1}, {1, 1}},
			Vertex{{1, -1, 0}, {0, 0, 1}, {1, 0}},
		},
		{0, 1, 2, 1, 2, 3});
}

Mesh generate_cube() {
	// clang-format off
		return Mesh(std::vector<Vertex> { 
			// back
			Vertex{ {-1, -1, -1}, {0, 0, -1}, {1, 0} }, Vertex{ {1, 1, -1}, {0, 0, -1}, {0, 1} },
				Vertex{ {1, -1, -1}, {0, 0, -1}, {0, 0} }, Vertex{ {1, 1, -1}, {0, 0, -1}, {0, 1} },
				Vertex{ {-1, -1, -1}, {0, 0, -1}, {1, 0} }, Vertex{ {-1, 1, -1}, {0, 0, -1}, {1, 1} },
				// front 
				Vertex{ {-1, -1, 1}, {0, 0, 1}, {0, 0} }, Vertex{ {1, -1, 1}, {0, 0, 1}, {1, 0} },
				Vertex{ {1, 1, 1}, {0, 0, 1}, {1, 1} }, Vertex{ {1, 1, 1}, {0, 0, 1}, {1, 1} },
				Vertex{ {-1, 1, 1}, {0, 0, 1}, {0, 1} }, Vertex{ {-1, -1, 1}, {0, 0, 1}, {0, 0} },
				// left 
				Vertex{ {-1, 1, -1}, {-1, 0, 0}, {0, 1} }, Vertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0} },
				Vertex{ {-1, 1, 1}, {-1, 0, 0}, {1, 1} }, Vertex{ {-1, -1, -1}, {-1, 0, 0}, {0, 0} },
				Vertex{ {-1, -1, 1}, {-1, 0, 0}, {1, 0} }, Vertex{ {-1, 1, 1}, {-1, 0, 0}, {1, 1} },
				// right 
				Vertex{ {1, 1, 1}, {1, 0, 0}, {0, 1} }, Vertex{ {1, -1, -1}, {1, 0, 0}, {1, 0} },
				Vertex{ {1, 1, -1}, {1, 0, 0}, {1, 1} }, Vertex{ {1, -1, -1}, {1, 0, 0}, {1, 0} },
				Vertex{ {1, 1, 1}, {1, 0, 0}, {0, 1} }, Vertex{ {1, -1, 1}, {1, 0, 0},  {0, 0} },
				// bottom 
				Vertex{ {-1, -1, -1}, {0, -1, 0}, {0, 0} }, Vertex{ {1, -1, -1}, {0, -1, 0}, {1, 0} },
				Vertex{ {1, -1, 1}, {0, -1, 0}, {1, 1} }, Vertex{ {1, -1, 1}, {0, -1, 0}, {1, 1} },
				Vertex{ {-1, -1, 1}, {0, -1, 0}, {0, 1} }, Vertex{ {-1, -1, -1}, {0, -1, 0}, {0, 0} },
				// top 
				Vertex{ {-1, 1, -1}, {0, 1, 0}, {0, 1} }, Vertex{ {1, 1, 1}, {0, 1, 0}, {1, 0} },
				Vertex{ {1, 1, -1}, {0, 1, 0}, {1, 1} }, Vertex{ {1, 1, 1}, {0, 1, 0}, {1, 0} },
				Vertex{ {-1, 1, -1}, {0, 1, 0}, {0, 1} }, Vertex{ {-1, 1, 1}, {0, 1, 0}, {0, 0} } },
			{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 });
	// clang-format on
}

Mesh generate_sphere(unsigned stackCount, unsigned sectorCount, float radius) {
	static constexpr unsigned int X_SEGMENTS = 64;
	static constexpr unsigned int Y_SEGMENTS = 64;
	static constexpr float PI = 3.14159265359;

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

			Vertex vert;
			vert.position = glm::vec3(xPos, yPos, zPos);
			vert.texCoord = glm::vec2(xSegment, ySegment);
			vert.normal = glm::vec3(xPos, yPos, zPos);

			vertices.push_back(vert);
		}
	}

	bool oddRow = false;
	for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
		if (!oddRow) {
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
			}
		} else {
			for (int x = X_SEGMENTS; x >= 0; --x) {
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
			}
		}
		oddRow = !oddRow;
	}

	return std::make_pair(vertices, indices);
}

vuk::Texture load_texture(std::string_view path, vuk::InflightContext& ifc, bool srgb = true) {
	auto resource = get_resource(path);
	int x, y, chans;
	auto img = stbi_load_from_memory(resource.data, resource.size, &x, &y, &chans, 4);
	if (img == nullptr) {
		spdlog::error("failed to load img: {}", path);
	}
	auto ptc = ifc.begin();
	auto [tex, _] = ptc.create_texture(srgb ? vuk::Format::eR8G8B8A8Srgb : vuk::Format::eR8G8B8A8Unorm, vuk::Extent3D{(unsigned)x, (unsigned)y, 1u}, img);
	ptc.wait_all_transfers();
	stbi_image_free(img);
	return std::move(tex);
}

vuk::Texture load_mipmapped_texture(std::string_view path, vuk::InflightContext& ifc, bool srgb = true) {
	auto resource = get_resource(path);
	int x, y, chans;
	auto img = stbi_load_from_memory(resource.data, resource.size, &x, &y, &chans, 4);
	if (img == nullptr) {
		spdlog::error("failed to load img: {}", path);
	}
	auto ptc = ifc.begin();

	u32 mips = (u32)std::min(std::log2f((f32)x), std::log2f((f32)y));

	vuk::ImageCreateInfo ici;
	ici.imageType = vuk::ImageType::e2D;
	ici.format = srgb ? vuk::Format::eR8G8B8A8Srgb : vuk::Format::eR8G8B8A8Unorm;
	ici.extent = vuk::Extent3D{(u32)x, (u32)y, 1u};
	ici.mipLevels = mips;
	ici.arrayLayers = 1;
	ici.usage = vuk::ImageUsageFlagBits::eSampled | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eTransferSrc;
	ici.tiling = vuk::ImageTiling::eOptimal;
	ici.initialLayout = vuk::ImageLayout::eUndefined;

	auto tex = ptc.allocate_texture(ici);

	ptc.upload(*tex.image, ici.format, ici.extent, 0, std::span(&img[0], x * y * 4), true);
	ptc.wait_all_transfers();

	stbi_image_free(img);
	return tex;
}

vuk::Texture load_cubemap_texture(std::string_view path, vuk::InflightContext& ifc) {
	auto resource = get_resource(path);
	int x, y, chans;
	stbi_set_flip_vertically_on_load(true);
	auto img = stbi_loadf_from_memory(resource.data, resource.size, &x, &y, &chans, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (img == nullptr) {
		spdlog::error("failed to load cubemap img: {}", path);
	}
	auto ptc = ifc.begin();

	vuk::ImageCreateInfo ici;
	ici.format = vuk::Format::eR32G32B32A32Sfloat;
	ici.extent = vuk::Extent3D{(unsigned)x, (unsigned)y, 1u};
	ici.samples = vuk::Samples::e1;
	ici.imageType = vuk::ImageType::e2D;
	ici.initialLayout = vuk::ImageLayout::eUndefined;
	ici.tiling = vuk::ImageTiling::eOptimal;
	ici.usage = vuk::ImageUsageFlagBits::eTransferSrc | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled;
	ici.mipLevels = 1;
	ici.arrayLayers = 1;

	auto tex = ptc.allocate_texture(ici);

	ptc.upload(*tex.image, ici.format, ici.extent, 0, std::span(&img[0], x * y * 4), false);

	ptc.wait_all_transfers();

	stbi_image_free(img);

	return tex;
}

vuk::Texture alloc_cubemap(unsigned width, unsigned height, vuk::InflightContext& ifc, uint32_t mips = 1) {
	vuk::ImageCreateInfo ici;
	ici.flags = vuk::ImageCreateFlagBits::eCubeCompatible;
	ici.imageType = vuk::ImageType::e2D;
	ici.format = vuk::Format::eR32G32B32A32Sfloat;
	ici.extent = vuk::Extent3D{width, height, 1u};
	ici.mipLevels = mips;
	ici.arrayLayers = 6;
	ici.usage = vuk::ImageUsageFlagBits::eColorAttachment | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled;

	auto ptc = ifc.begin();
	auto texture = ptc.allocate_texture(ici);

	return texture;
}

vuk::Texture alloc_lut(unsigned width, unsigned height, vuk::InflightContext& ifc) {
	vuk::ImageCreateInfo ici;
	ici.imageType = vuk::ImageType::e2D;
	ici.format = vuk::Format::eR16G16Sfloat;
	ici.extent = vuk::Extent3D{width, height, 1u};
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.usage = vuk::ImageUsageFlagBits::eColorAttachment | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled;

	auto ptc = ifc.begin();
	auto texture = ptc.allocate_texture(ici);

	return texture;
}

struct Uniforms {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
};

class PBR {
  public:
	void setup(vuk::Context& context, vuk::InflightContext& ifc) {
		vuk::PipelineBaseCreateInfo pbr;
		pbr.add_shader(get_resource_string("Resources/Shaders/pbr.vert"), "pbr.vert");
		pbr.add_shader(get_resource_string("Resources/Shaders/pbr.frag"), "pbr.frag");
		context.create_named_pipeline("pbr", pbr);

		vuk::PipelineBaseCreateInfo equirectangular_to_cubemap;
		equirectangular_to_cubemap.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
		equirectangular_to_cubemap.add_shader(get_resource_string("Resources/Shaders/equirectangular_to_cubemap.frag"), "equirectangular_to_cubemap.frag");
		context.create_named_pipeline("equirectangular_to_cubemap", equirectangular_to_cubemap);

		vuk::PipelineBaseCreateInfo irradiance;
		irradiance.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
		irradiance.add_shader(get_resource_string("Resources/Shaders/irradiance_convolution.frag"), "irradiance_convolution.frag");
		context.create_named_pipeline("irradiance", irradiance);

		vuk::PipelineBaseCreateInfo prefilter;
		prefilter.add_shader(get_resource_string("Resources/Shaders/cubemap.vert"), "cubemap.vert");
		prefilter.add_shader(get_resource_string("Resources/Shaders/prefilter.frag"), "prefilter.frag");
		context.create_named_pipeline("prefilter", prefilter);

		vuk::PipelineBaseCreateInfo brdf;
		brdf.add_shader(get_resource_string("Resources/Shaders/brdf.vert"), "brdf.vert");
		brdf.add_shader(get_resource_string("Resources/Shaders/brdf.frag"), "brdf.frag");
		context.create_named_pipeline("brdf", brdf);

		sphere = generate_sphere(36, 36, 1.f);
		cube = generate_cube();
		quad = generate_quad();

		albedo_texture = load_mipmapped_texture("Resources/Textures/rust_albedo.jpg", ifc);
		metallic_texture = load_mipmapped_texture("Resources/Textures/rust_metallic.png", ifc, false);
		normal_texture = load_mipmapped_texture("Resources/Textures/rust_normal.jpg", ifc, false);
		roughness_texture = load_mipmapped_texture("Resources/Textures/rust_roughness.jpg", ifc, true);
		ao_texture = load_mipmapped_texture("Resources/Textures/rust_ao.jpg", ifc, false);

		hdr_texture = load_cubemap_texture("Resources/Textures/forest_slope_1k.hdr", ifc);

		// the HDR has been loaded as a 2:1 equirectangular, now it needs to be converted to a
		// cubemap

		env_cubemap = std::make_pair(alloc_cubemap(512, 512, ifc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
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

		auto ptc = ifc.begin();

		auto [bverts, stub1] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span(&cube.first[0], cube.first.size()));
		auto verts = std::move(bverts);
		auto [binds, stub2] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span(&cube.second[0], cube.second.size()));
		auto inds = std::move(binds);

		ptc.wait_all_transfers();

		vuk::ImageViewCreateInfo ivci{
			.image = *env_cubemap->first.image,
			.viewType = vuk::ImageViewType::e2D,
			.format = vuk::Format::eR32G32B32A32Sfloat,
		};
		ivci.subresourceRange.aspectMask = vuk::ImageAspectFlagBits::eColor;
		ivci.subresourceRange.layerCount = 1;

		for (unsigned i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"env_cubemap_face"_image(vuk::eColorWrite)},
						 .execute = [verts, inds, capture_projection, capture_views, i, this](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Area::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Area::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, hdr_texture,
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
							 cbuf.draw_indexed(cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			rg.bind_attachment("env_cubemap_face",
							   vuk::Attachment{
								   .image = *env_cubemap->first.image,
								   .image_view = *iv,
								   .extent = vuk::Extent2D{512, 512},
								   .format = vuk::Format::eR32G32B32A32Sfloat,
							   },
							   vuk::Access::eNone, vuk::Access::eFragmentSampled);

			rg.build();
			rg.build(ptc);

			vuk::execute_submit_and_wait(ptc, rg);
		}

		env_cubemap_iv = ptc.create_image_view(
			vuk::ImageViewCreateInfo{.image = *env_cubemap->first.image,
									 .viewType = vuk::ImageViewType::eCube,
									 .format = vuk::Format::eR32G32B32A32Sfloat,
									 .subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 6}});

		irradiance_cubemap = std::make_pair(alloc_cubemap(32, 32, ifc), vuk::SamplerCreateInfo{.magFilter = vuk::Filter::eLinear,
																							   .minFilter = vuk::Filter::eLinear,
																							   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																							   .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																							   .addressModeW = vuk::SamplerAddressMode::eClampToEdge});

		ivci.image = *irradiance_cubemap->first.image;

		env_cubemap->second.addressModeU = vuk::SamplerAddressMode::eClampToEdge;
		env_cubemap->second.addressModeV = vuk::SamplerAddressMode::eClampToEdge;
		env_cubemap->second.addressModeW = vuk::SamplerAddressMode::eClampToEdge;

		for (unsigned i = 0; i < 6; ++i) {
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"irradiance_cubemap_face"_image(vuk::eColorWrite)},
						 .execute = [verts, inds, capture_projection, capture_views, i, this](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Area::absolute(0, 0, 32, 32))
								 .set_scissor(0, vuk::Area::absolute(0, 0, 32, 32))
								 .bind_vertex_buffer(0, verts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(inds, vuk::IndexType::eUint32)
								 .bind_sampled_image(0, 2, *env_cubemap_iv, env_cubemap->second)
								 .bind_graphics_pipeline("irradiance");
							 glm::mat4* projection = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 0);
							 *projection = capture_projection;
							 glm::mat4* view = cbuf.map_scratch_uniform_binding<glm::mat4>(0, 1);
							 *view = capture_views[i];
							 cbuf.draw_indexed(cube.second.size(), 1, 0, 0, 0);
						 }});

			ivci.subresourceRange.baseArrayLayer = i;
			auto iv = ptc.create_image_view(ivci);

			vuk::Attachment attachment;
			attachment.image = *irradiance_cubemap->first.image;
			attachment.image_view = *iv;
			attachment.extent = vuk::Extent2D{32, 32};
			attachment.format = vuk::Format::eR32G32B32A32Sfloat;

			rg.bind_attachment("irradiance_cubemap_face", attachment, vuk::Access::eNone, vuk::Access::eFragmentSampled);

			rg.build();
			rg.build(ptc);

			vuk::execute_submit_and_wait(ptc, rg);
		}

		vuk::ImageSubresourceRange img_subres_range;
		img_subres_range.aspectMask = vuk::ImageAspectFlagBits::eColor;
		img_subres_range.layerCount = 6;

		vuk::ImageViewCreateInfo ivci_irradiance;
		ivci_irradiance.image = *irradiance_cubemap->first.image;
		ivci_irradiance.viewType = vuk::ImageViewType::eCube;
		ivci_irradiance.format = vuk::Format::eR32G32B32A32Sfloat;
		ivci_irradiance.subresourceRange = img_subres_range;

		irradiance_cubemap_iv = ptc.create_image_view(ivci_irradiance);

		vuk::SamplerCreateInfo sci;
		sci.magFilter = vuk::Filter::eLinear;
		sci.minFilter = vuk::Filter::eLinear;
		sci.mipmapMode = vuk::SamplerMipmapMode::eLinear;
		sci.addressModeU = vuk::SamplerAddressMode::eClampToEdge;
		sci.addressModeV = vuk::SamplerAddressMode::eClampToEdge;
		sci.addressModeW = vuk::SamplerAddressMode::eClampToEdge;
		sci.minLod = 0.f;
		sci.maxLod = 4.f;

		prefilter_cubemap = std::make_pair(alloc_cubemap(128, 128, ifc, 5), sci);

		ivci.image = *prefilter_cubemap->first.image;

		u32 max_mip = 5;
		for (unsigned mip = 0; mip < max_mip; ++mip) {
			uint32_t mip_wh = 128 * pow(0.5, mip);
			float roughness = (float)mip / (float)(max_mip - 1);

			ivci.subresourceRange.baseMipLevel = mip;

			for (unsigned i = 0; i < 6; ++i) {
				vuk::RenderGraph rg;
				rg.add_pass({.resources = {"prefilter_cubemap_face"_image(vuk::eColorWrite)},
							 .execute = [roughness, mip_wh, verts, inds, capture_projection, capture_views, i, this](vuk::CommandBuffer& cbuf) {
								 cbuf.set_viewport(0, vuk::Area::absolute(0, 0, mip_wh, mip_wh))
									 .set_scissor(0, vuk::Area::absolute(0, 0, mip_wh, mip_wh))
									 .bind_vertex_buffer(0, verts, 0,
														 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
									 .bind_index_buffer(inds, vuk::IndexType::eUint32)
									 .bind_sampled_image(0, 2, *env_cubemap_iv,
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
								 cbuf.draw_indexed(cube.second.size(), 1, 0, 0, 0);
							 }});

				ivci.subresourceRange.baseArrayLayer = i;
				auto iv = ptc.create_image_view(ivci);

				vuk::Attachment attachment;
				attachment.image = *prefilter_cubemap->first.image;
				attachment.image_view = *iv;
				attachment.extent = vuk::Extent2D{mip_wh, mip_wh};
				attachment.format = vuk::Format::eR32G32B32A32Sfloat;

				rg.bind_attachment("prefilter_cubemap_face", attachment, vuk::Access::eNone, vuk::Access::eFragmentSampled);

				rg.build();
				rg.build(ptc);

				vuk::execute_submit_and_wait(ptc, rg);
			}
		}

		prefilter_cubemap_iv = ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *prefilter_cubemap->first.image,
			.viewType = vuk::ImageViewType::eCube,
			.format = vuk::Format::eR32G32B32A32Sfloat,
			.subresourceRange = vuk::ImageSubresourceRange{.aspectMask = vuk::ImageAspectFlagBits::eColor, .layerCount = 6, .levelCount = 4}});

		brdf_lut = std::make_pair(alloc_lut(512, 512, ifc), vuk::SamplerCreateInfo{
																.magFilter = vuk::Filter::eLinear,
																.minFilter = vuk::Filter::eLinear,
																.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
																.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
																.addressModeW = vuk::SamplerAddressMode::eClampToEdge,
															});

		ivci.image = *brdf_lut->first.image;
		ivci.format = vuk::Format::eR16G16Sfloat;
		ivci.subresourceRange = {};
		ivci.subresourceRange.aspectMask = vuk::ImageAspectFlagBits::eColor;

		auto [bqverts, _s1] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span(&quad.first[0], quad.first.size()));
		auto qverts = std::move(bqverts);
		auto [qbinds, _s2] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span(&quad.second[0], quad.second.size()));
		auto qinds = std::move(qbinds);

		ptc.wait_all_transfers();

		{
			vuk::RenderGraph rg;
			rg.add_pass({.resources = {"brdf_out"_image(vuk::eColorWrite)}, .execute = [qverts, qinds, this](vuk::CommandBuffer& cbuf) {
							 cbuf.set_viewport(0, vuk::Area::absolute(0, 0, 512, 512))
								 .set_scissor(0, vuk::Area::absolute(0, 0, 512, 512))
								 .bind_vertex_buffer(0, qverts, 0,
													 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
								 .bind_index_buffer(qinds, vuk::IndexType::eUint32)
								 .bind_graphics_pipeline("brdf");
							 cbuf.draw_indexed(quad.second.size(), 1, 0, 0, 0);
						 }});

			auto iv = ptc.create_image_view(ivci);

			rg.bind_attachment("brdf_out",
							   vuk::Attachment{
								   .image = *brdf_lut->first.image,
								   .image_view = *iv,
								   .extent = vuk::Extent2D{512, 512},
								   .format = vuk::Format::eR16G16Sfloat,
							   },
							   vuk::Access::eNone, vuk::Access::eFragmentSampled);

			rg.build();
			rg.build(ptc);

			vuk::execute_submit_and_wait(ptc, rg);
		}
	}

	vuk::RenderGraph render(vuk::Context& context, vuk::InflightContext& ifc) {
		auto ptc = ifc.begin();

		auto [bverts, stub1] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eVertexBuffer, std::span(&sphere.first[0], sphere.first.size()));
		auto verts = std::move(bverts);
		auto [binds, stub2] =
			ptc.create_scratch_buffer(vuk::MemoryUsage::eGPUonly, vuk::BufferUsageFlagBits::eIndexBuffer, std::span(&sphere.second[0], sphere.second.size()));
		auto inds = std::move(binds);

		Uniforms uniforms;

		uniforms.projection = glm::perspective(glm::degrees(60.f), 800.f / 600.f, 1.f, 10.f);
		uniforms.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0), glm::vec3(0, 1, 0));
		uniforms.model = static_cast<glm::mat4>(glm::angleAxis(glm::radians(angle), glm::vec3(0.f, 1.f, 0.f))) *
						 static_cast<glm::mat4>(glm::angleAxis(glm::degrees(90.f), glm::vec3(1.f, 0.f, 0.f)));

		auto [bubo, stub3] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span(&uniforms, 1));
		auto ubo = bubo;
		ptc.wait_all_transfers();

		u32 mips = (u32)std::min(std::log2f(1024.f), std::log2f(1024.f));

		vuk::SamplerCreateInfo map_sampler{.magFilter = vuk::Filter::eLinear,
										   .minFilter = vuk::Filter::eLinear,
										   .addressModeU = vuk::SamplerAddressMode::eClampToEdge,
										   .addressModeV = vuk::SamplerAddressMode::eClampToEdge,
										   .addressModeW = vuk::SamplerAddressMode::eClampToEdge,
										   //.anisotropyEnable = VK_TRUE,
										   //.maxAnisotropy = 16.f,
										   .minLod = 0.f,
										   .maxLod = (f32)mips};

		vuk::RenderGraph rg;
		rg.add_pass({.resources = {"pbr_msaa"_image(vuk::eColorWrite), "pbr_depth"_image(vuk::eDepthStencilRW)},
					 .execute = [map_sampler, verts, ubo, inds, this](vuk::CommandBuffer& cbuf) {
						 cbuf.set_viewport(0, vuk::Area::framebuffer())
							 .set_scissor(0, vuk::Area::framebuffer())
							 .set_primitive_topology(vuk::PrimitiveTopology::eTriangleStrip)
							 .bind_vertex_buffer(0, verts, 0,
												 vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32Sfloat})
							 .bind_index_buffer(inds, vuk::IndexType::eUint32)
							 .bind_sampled_image(0, 1, *albedo_texture, map_sampler)
							 .bind_sampled_image(0, 2, *normal_texture, map_sampler)
							 .bind_sampled_image(0, 3, *metallic_texture, map_sampler)
							 .bind_sampled_image(0, 4, *roughness_texture, map_sampler)
							 .bind_sampled_image(0, 5, *ao_texture, map_sampler)
							 .bind_sampled_image(0, 6, *irradiance_cubemap_iv, irradiance_cubemap->second)
							 .bind_sampled_image(0, 7, *prefilter_cubemap_iv, prefilter_cubemap->second)
							 .bind_sampled_image(0, 8, brdf_lut->first, brdf_lut->second)
							 .bind_graphics_pipeline("pbr")
							 .bind_uniform_buffer(0, 0, ubo);
						 cbuf.draw_indexed(sphere.second.size(), 1, 0, 0, 0);
					 }});

		rg.mark_attachment_internal("pbr_msaa", vuk::Format::eR8G8B8A8Srgb, vuk::Extent2D::Framebuffer{}, vuk::Samples::e8,
									vuk::ClearColor{0.01f, 0.01f, 0.01f, 1.f});
		rg.mark_attachment_internal("pbr_depth", vuk::Format::eD32Sfloat, vuk::Extent2D::Framebuffer{}, vuk::Samples::Framebuffer{},
									vuk::ClearDepthStencil{1.0f, 0});
		rg.mark_attachment_resolve("pbr_final", "pbr_msaa");

		return rg;
	}

	void cleanup() {
		albedo_texture.reset();
		metallic_texture.reset();
		normal_texture.reset();
		roughness_texture.reset();
		ao_texture.reset();

		env_cubemap.reset();
		irradiance_cubemap.reset();
		prefilter_cubemap.reset();
		brdf_lut.reset();

		env_cubemap_iv.reset();
		irradiance_cubemap_iv.reset();
		prefilter_cubemap_iv.reset();
	}

	float angle;

  private:
	Mesh sphere;
	Mesh cube;
	Mesh quad;

	vuk::Texture hdr_texture;

	std::optional<vuk::Texture> albedo_texture;
	std::optional<vuk::Texture> metallic_texture;
	std::optional<vuk::Texture> normal_texture;
	std::optional<vuk::Texture> roughness_texture;
	std::optional<vuk::Texture> ao_texture;

	std::optional<std::pair<vuk::Texture, vuk::SamplerCreateInfo>> env_cubemap;
	std::optional<std::pair<vuk::Texture, vuk::SamplerCreateInfo>> irradiance_cubemap;
	std::optional<std::pair<vuk::Texture, vuk::SamplerCreateInfo>> prefilter_cubemap;
	std::optional<std::pair<vuk::Texture, vuk::SamplerCreateInfo>> brdf_lut;
	vuk::Unique<vuk::ImageView> env_cubemap_iv;
	vuk::Unique<vuk::ImageView> irradiance_cubemap_iv;
	vuk::Unique<vuk::ImageView> prefilter_cubemap_iv;
};

int main() {
	if (!glfwInit()) {
		spdlog::error("failed to init glfw");
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(800, 600, "PBR", nullptr, nullptr);

	vkb::InstanceBuilder builder;
	builder.request_validation_layers()
		.use_default_debug_messenger()
		.set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
							   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) -> VkBool32 {
			auto ms = vkb::to_string_message_severity(messageSeverity);
			auto mt = vkb::to_string_message_type(messageType);
			spdlog::error("[{}: {}](user defined)\n{}", ms, mt, pCallbackData->pMessage);
			return VK_FALSE;
		})
		.set_app_name("PBR")
		.set_engine_name("PBRvuk")
		.require_api_version(1, 2, 0)
		.set_app_version(1, 0, 0);

	auto inst_ret = builder.build();
	auto vkb_instance = inst_ret.value();
	auto instance = vkb_instance.instance;

	VkPhysicalDeviceFeatures phys_dev_features = {};
	phys_dev_features.samplerAnisotropy = VK_TRUE;

	VkSurfaceKHR surface;
	glfwCreateWindowSurface(instance, window, nullptr, &surface);
	vkb::PhysicalDeviceSelector selector{vkb_instance};
	selector.set_surface(surface).set_minimum_version(1, 2);
	selector.set_required_features(phys_dev_features);
	auto phys_ret = selector.select();
	if (!phys_ret.has_value()) {
		spdlog::error("failed to select physical device");
		return -1;
	}
	vkb::PhysicalDevice vkb_physical_device = phys_ret.value();
	auto physical_device = vkb_physical_device.physical_device;

	vkb::DeviceBuilder device_builder{vkb_physical_device};
	VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES};
	descriptor_indexing_features.descriptorBindingPartiallyBound = true;
	descriptor_indexing_features.descriptorBindingUpdateUnusedWhilePending = true;
	descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = true;
	descriptor_indexing_features.runtimeDescriptorArray = true;
	descriptor_indexing_features.descriptorBindingVariableDescriptorCount = true;
	VkPhysicalDeviceVulkan11Features feats{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
	feats.shaderDrawParameters = true;
	auto dev_ret = device_builder.add_pNext(&descriptor_indexing_features).add_pNext(&feats).build();
	if (!dev_ret.has_value()) {
		spdlog::error("failed to build device");
		return -1;
	}
	auto vkb_device = dev_ret.value();
	auto graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
	auto device = vkb_device.device;

	vkb::SwapchainBuilder swb(vkb_device);
	swb.set_desired_format(vuk::SurfaceFormatKHR{vuk::Format::eR8G8B8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.add_fallback_format(vuk::SurfaceFormatKHR{vuk::Format::eB8G8R8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.set_desired_present_mode((VkPresentModeKHR)vuk::PresentModeKHR::eImmediate);
	swb.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	auto vk_swapchain = swb.build();

	vuk::Swapchain sw;
	auto images = vk_swapchain->get_images();
	auto views = vk_swapchain->get_image_views();

	for (auto& i : *images) {
		sw.images.push_back(i);
	}
	for (auto& i : *views) {
		sw.image_views.emplace_back();
		sw.image_views.back().payload = i;
	}
	sw.extent = vuk::Extent2D{vk_swapchain->extent.width, vk_swapchain->extent.height};
	sw.format = vuk::Format(vk_swapchain->image_format);
	sw.surface = vkb_device.surface;
	sw.swapchain = vk_swapchain->swapchain;

	std::optional<vuk::Context> context;
	context.emplace(instance, device, physical_device, graphics_queue);

	auto vuk_swapchain = context->add_swapchain(sw);

	PBR pbr;

	{
		auto ifc = context->begin();
		pbr.setup(*context, ifc);
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		pbr.angle += 0.01f;

		auto ifc = context->begin();
		auto rg = pbr.render(*context, ifc);
		rg.build();
		rg.bind_attachment_to_swapchain("pbr_final", vuk_swapchain, vuk::ClearColor{0.01f, 0.01f, 0.01f, 1.f});
		auto ptc = ifc.begin();
		rg.build(ptc);

		vuk::execute_submit_and_present_to_one(ptc, rg, vuk_swapchain);
	}

	context.reset();
	vkDestroySurfaceKHR(vkb_instance.instance, surface, nullptr);
	vkb::destroy_device(vkb_device);
	vkb::destroy_instance(vkb_instance);

	glfwDestroyWindow(window);
	glfwTerminate();
}
