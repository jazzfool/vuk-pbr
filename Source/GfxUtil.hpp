#pragma once

#include "Types.hpp"

#include <string_view>
#include <vuk/Context.hpp>

struct Context;

namespace gfx_util {

vuk::Texture load_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb = true);
vuk::Texture load_mipmapped_texture(std::string_view path, vuk::PerThreadContext& ptc, bool srgb = true);
vuk::Texture load_cubemap_texture(std::string_view path, vuk::PerThreadContext& ptc);
vuk::Texture alloc_cubemap(u32 width, u32 height, vuk::PerThreadContext& ptc, u32 mips = 1);
vuk::Texture alloc_lut(u32 width, u32 height, vuk::PerThreadContext& ptc);

u64 uniform_buffer_offset_alignment(struct Context& ctxt, u64 min);

template <typename T>
u64 uniform_buffer_offset_alignment(Context& ctxt) {
	return uniform_buffer_offset_alignment(ctxt, sizeof(T));
}

} // namespace gfx_util