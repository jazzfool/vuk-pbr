#include "Context.hpp"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <vuk/Context.hpp>

std::optional<Context> Context::create() {
	if (!glfwInit()) {
		spdlog::error("failed to init glfw");
		return {};
	}

	Context ctxt;

	// don't let GLFW create an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	ctxt.window = glfwCreateWindow(800, 600, "Vuk PBR", nullptr, nullptr);

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
		.set_app_name("Vuk PBR")
		.set_engine_name("PBRVuk")
		.require_api_version(1, 2, 0)
		.set_app_version(1, 0, 0);

	auto inst_ret = builder.build();
	ctxt.vkb_instance = inst_ret.value();
	ctxt.instance = ctxt.vkb_instance.instance;

	VkPhysicalDeviceFeatures phys_dev_features = {};
	phys_dev_features.samplerAnisotropy = VK_TRUE;
	phys_dev_features.depthClamp = VK_TRUE;

	glfwCreateWindowSurface(ctxt.instance, ctxt.window, nullptr, &ctxt.surface);

	vkb::PhysicalDeviceSelector selector{ctxt.vkb_instance};
	selector.set_surface(ctxt.surface).set_minimum_version(1, 2);
	selector.set_required_features(phys_dev_features);

	auto phys_ret = selector.select();
	if (!phys_ret.has_value()) {
		spdlog::error("failed to select physical device");
		return {};
	}

	ctxt.vkb_physical_device = phys_ret.value();
	ctxt.physical_device = ctxt.vkb_physical_device.physical_device;

	vkb::DeviceBuilder device_builder{ctxt.vkb_physical_device};

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
		return {};
	}

	ctxt.vkb_device = dev_ret.value();
	ctxt.graphics_queue = ctxt.vkb_device.get_queue(vkb::QueueType::graphics).value();
	ctxt.device = ctxt.vkb_device.device;

	vkb::SwapchainBuilder swb(ctxt.vkb_device);
	swb.set_desired_format(vuk::SurfaceFormatKHR{vuk::Format::eR8G8B8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.add_fallback_format(vuk::SurfaceFormatKHR{vuk::Format::eB8G8R8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.set_desired_present_mode((VkPresentModeKHR)vuk::PresentModeKHR::eImmediate);
	swb.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	auto vk_swapchain = swb.build();
	ctxt.vkb_swapchain = *vk_swapchain;

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
	sw.surface = ctxt.vkb_device.surface;
	sw.swapchain = vk_swapchain->swapchain;

	ctxt.vuk_context = std::make_unique<vuk::Context>(ctxt.instance, ctxt.device, ctxt.physical_device, ctxt.graphics_queue);

	ctxt.vuk_swapchain = ctxt.vuk_context->add_swapchain(sw);

	return ctxt;
}

void Context::cleanup(std::optional<Context>& ctxt) {
	ctxt->vuk_context->wait_idle();
	for (auto i = 0; i < vuk::Context::FC; ++i) {
		ctxt->vuk_context->begin();
	}

	auto vkb_instance = ctxt->vkb_instance;
	auto vkb_device = ctxt->vkb_device;
	auto surface = ctxt->surface;
	auto window = ctxt->window;

	ctxt.reset();

	vkDestroySurfaceKHR(vkb_instance.instance, surface, nullptr);
	vkb::destroy_device(vkb_device);
	vkb::destroy_instance(vkb_instance);

	glfwDestroyWindow(window);
	glfwTerminate();
}
