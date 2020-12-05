#pragma once

#include "Cache.hpp"

#include <vuk/Image.hpp>

using TextureCache = Cache<std::string_view, vuk::Texture>;

struct Material {
	TextureCache::View albedo;
	TextureCache::View metallic;
	TextureCache::View roughness;
	TextureCache::View normal;
	TextureCache::View ao;
};
