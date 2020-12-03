#include "Resource.hpp"

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(vpbr);

Resource get_resource(std::string_view path) {
	static const auto res = cmrc::vpbr::get_filesystem();
	auto r = res.open(path.data());
	return Resource{
		reinterpret_cast<const u8*>(r.cbegin()),
		r.size(),
	};
}

std::string get_resource_string(std::string_view path) {
	auto res = get_resource(path);
	return std::string{reinterpret_cast<const char*>(res.data)};
}
