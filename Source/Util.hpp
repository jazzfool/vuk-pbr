#pragma once

#include <utility>
#include <istream>

struct MemoryBuf : std::streambuf {
	MemoryBuf(char const* base, size_t size) {
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
};

struct IMemoryStream : virtual MemoryBuf, std::istream {
	IMemoryStream(char const* base, size_t size) : MemoryBuf(base, size), std::istream(static_cast<std::streambuf*>(this)) {
	}
};

inline void hash_combine(std::size_t& seed) {
}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	hash_combine(seed, rest...);
}
