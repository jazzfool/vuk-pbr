#include "GfxUtil.hpp"

#include "Resource.hpp"

#include <stb_image/stb_image.h>
#include <spdlog/spdlog.h>

namespace gfx_util {

vuk::Texture load_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb) {
	auto resource = get_resource(path);
	i32 x, y, chans;
	auto img = stbi_load_from_memory(resource.data, resource.size, &x, &y, &chans, 4);
	if (img == nullptr) {
		spdlog::error("failed to load img: {}", path);
	}
	auto [tex, _] = ptc.create_texture(srgb ? vuk::Format::eR8G8B8A8Srgb : vuk::Format::eR8G8B8A8Unorm, vuk::Extent3D{(u32)x, (u32)y, 1u}, img);
	ptc.wait_all_transfers();
	stbi_image_free(img);
	return std::move(tex);
}

vuk::Texture load_mipmapped_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb) {
	auto resource = get_resource(path);
	i32 x, y, chans;
	auto img = stbi_load_from_memory(resource.data, resource.size, &x, &y, &chans, 4);
	if (img == nullptr) {
		spdlog::error("failed to load img: {}", path);
	}

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

vuk::Texture load_cubemap_texture(std::string_view path, vuk::PerThreadContext& ptc) {
	auto resource = get_resource(path);
	i32 x, y, chans;
	stbi_set_flip_vertically_on_load(true);
	auto img = stbi_loadf_from_memory(resource.data, resource.size, &x, &y, &chans, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (img == nullptr) {
		spdlog::error("failed to load cubemap img: {}", path);
	}

	vuk::ImageCreateInfo ici;
	ici.format = vuk::Format::eR32G32B32A32Sfloat;
	ici.extent = vuk::Extent3D{(u32)x, (u32)y, 1u};
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

vuk::Texture alloc_cubemap(u32 width, u32 height, vuk::PerThreadContext& ptc, u32 mips) {
	vuk::ImageCreateInfo ici;
	ici.flags = vuk::ImageCreateFlagBits::eCubeCompatible;
	ici.imageType = vuk::ImageType::e2D;
	ici.format = vuk::Format::eR32G32B32A32Sfloat;
	ici.extent = vuk::Extent3D{width, height, 1u};
	ici.mipLevels = mips;
	ici.arrayLayers = 6;
	ici.usage = vuk::ImageUsageFlagBits::eColorAttachment | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled;

	auto texture = ptc.allocate_texture(ici);

	return texture;
}

vuk::Texture alloc_lut(u32 width, u32 height, vuk::PerThreadContext& ptc) {
	vuk::ImageCreateInfo ici;
	ici.imageType = vuk::ImageType::e2D;
	ici.format = vuk::Format::eR16G16Sfloat;
	ici.extent = vuk::Extent3D{width, height, 1u};
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.usage = vuk::ImageUsageFlagBits::eColorAttachment | vuk::ImageUsageFlagBits::eTransferDst | vuk::ImageUsageFlagBits::eSampled;

	auto texture = ptc.allocate_texture(ici);

	return texture;
}

}
