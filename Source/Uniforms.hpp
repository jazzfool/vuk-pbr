#pragma once

#include "GfxParts/CascadedShadows.hpp"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/Context.hpp>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <utility>

struct alignas(16) FrameUniforms {
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec4 cam_pos;
	glm::vec2 screen_size;
	glm::vec2 clip_range;
};

struct alignas(16) ShadowUniforms {
	glm::vec4 cascade_splits;
	glm::mat4 cascade_view_proj_mats[CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT];
};

struct UniformStore {
	std::unordered_map<std::type_index, std::pair<vuk::Buffer, std::any>> uniforms;

	template <typename T>
	bool contains() const {
		return uniforms.count(typeid(T)) == 1;
	}

	template <typename T>
	void insert(T val) {
		uniforms.insert(typeid(T), val);
	}

	template <typename T>
	void emplace(T&& val) {
		uniforms.emplace(typeid(T), std::move(val));
	}

	template <typename T>
	T& data() {
		return std::any_cast<T>(uniforms.at(typeid(T)).second);
	}

	template <typename T>
	const T& data() const {
		return std::any_cast<T>(uniforms.at(typeid(T)).second);
	}

	template <typename T>
	vuk::Buffer buffer() const {
		return uniforms.at(typeid(T)).first;
	}

	template <typename T>
	void upload(vuk::PerThreadContext& ptc) const {
		auto buf = buffer<T>();
		const auto& d = data<T>();
		ptc.upload(buf, std::span{&d, 1});
	}
};
