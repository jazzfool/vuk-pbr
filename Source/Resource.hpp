#pragma once

#include "Types.hpp"

#include <string_view>

struct Resource {
	const u8* data;
	u64 size;
};

Resource get_resource(std::string_view path);
std::string get_resource_string(std::string_view path);
