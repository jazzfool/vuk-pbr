#pragma once

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vuk/Image.hpp>
#include <vuk/Swapchain.hpp>
#include <vuk/Context.hpp>
#include <optional>
#include <memory>

struct Context {
	static std::optional<Context> create();
	static void cleanup(std::optional<Context>& ctxt);

	struct GLFWwindow* window;

	vkb::Instance vkb_instance;
	VkInstance instance;

	vkb::PhysicalDevice vkb_physical_device;
	VkPhysicalDevice physical_device;

	vkb::Device vkb_device;
	VkDevice device;
	VkQueue graphics_queue;

	VkSurfaceKHR surface;
	vkb::Swapchain vkb_swapchain;
	vuk::Swapchain* vuk_swapchain;

	std::unique_ptr<vuk::Context> vuk_context;
};
