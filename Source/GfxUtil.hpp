#pragma once

#include "Types.hpp"

#include <string_view>
#include <vuk/Context.hpp>

namespace gfx_util {

vuk::Texture load_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb = true);
vuk::Texture load_mipmapped_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb = true);
vuk::Texture load_cubemap_texture(std::string_view path, vuk::PerThreadContext& ptc);
vuk::Texture alloc_cubemap(u32 width, u32 height, vuk::PerThreadContext& ptc, u32 mips = 1);
vuk::Texture alloc_lut(u32 width, u32 height, vuk::PerThreadContext& ptc);

} // namespace gfx_util