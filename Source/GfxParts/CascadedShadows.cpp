#include "CascadedShadows.hpp"

#include "../Scene.hpp"
#include "../Resource.hpp"
#include "../Context.hpp"
#include "../Mesh.hpp"
#include "../Renderer.hpp"

#include <vuk/CommandBuffer.hpp>
#include <limits>

CascadedShadowRenderPass::CascadedShadowRenderPass() : cascade_split_lambda{0.95f} {
}

void CascadedShadowRenderPass::debug(vuk::CommandBuffer& cbuf, u8 cascade) {
	cbuf.set_viewport(0, vuk::Rect2D::framebuffer())
		.set_scissor(0, vuk::Rect2D::framebuffer())
		.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
		.bind_graphics_pipeline("debug")
		.bind_sampled_image(0, 0, m_shadow_map, {})
		.draw(3, 1, 0, 0);
}

void CascadedShadowRenderPass::init(vuk::PerThreadContext& ptc, struct Context& ctxt, struct UniformStore& uniforms, PipelineStore& ps) {
	vuk::PipelineBaseCreateInfo depth_pipe;
	depth_pipe.depth_stencil_state.depthCompareOp = vuk::CompareOp::eLessOrEqual;
	depth_pipe.rasterization_state.depthClampEnable = VK_TRUE;
	depth_pipe.rasterization_state.cullMode = vuk::CullModeFlagBits::eFront;
	ps.add("depth_only", "depth_only.vert", "depth_only.frag", depth_pipe);

	ps.add("debug_shadow_map", "debug_shadow_map.vert", "debug_shadow_map.frag");

	m_shadow_map = ctxt.vuk_context->allocate_texture(vuk::ImageCreateInfo{
		.imageType = vuk::ImageType::e2D,
		.format = vuk::Format::eD32Sfloat,
		.extent = vuk::Extent3D{CascadedShadowRenderPass::DIMENSION, CascadedShadowRenderPass::DIMENSION, 1},
		.mipLevels = 1,
		.arrayLayers = CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT,
		.samples = vuk::SampleCountFlagBits::e1,
		.tiling = vuk::ImageTiling::eOptimal,
		.usage = vuk::ImageUsageFlagBits::eSampled | vuk::ImageUsageFlagBits::eDepthStencilAttachment,
		.sharingMode = vuk::SharingMode::eExclusive,
	});

	m_shadow_map_view = ptc.create_image_view(vuk::ImageViewCreateInfo{
		.image = *m_shadow_map.image,
		.viewType = vuk::ImageViewType::e2DArray,
		.format = vuk::Format::eD32Sfloat,
		.subresourceRange =
			vuk::ImageSubresourceRange{
				.aspectMask = vuk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT,
			},
	});

	for (u8 i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i) {
		std::string name = "depth_map_layer_";
		name = name.append(std::to_string(i));
		m_attachment_names.push_back(name);

		m_image_views.push_back(ptc.create_image_view(vuk::ImageViewCreateInfo{
			.image = *m_shadow_map.image,
			.viewType = vuk::ImageViewType::e2DArray,
			.format = vuk::Format::eD32Sfloat,
			.subresourceRange =
				vuk::ImageSubresourceRange{
					.aspectMask = vuk::ImageAspectFlagBits::eDepth,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = static_cast<u32>(i),
					.layerCount = 1,
				},
		}));
	}
}

void CascadedShadowRenderPass::prep(vuk::PerThreadContext& ptc, struct Context& ctxt, struct RenderInfo& info) {
}

void CascadedShadowRenderPass::render(
	vuk::PerThreadContext& ptc, struct Context& ctxt, vuk::RenderGraph& rg, const class SceneRenderer& renderer, struct RenderInfo& info) {
	const auto cascades = compute_cascades(info);
	std::vector<glm::mat4> cascade_mats;
	cascade_mats.reserve(cascades.size());
	for (auto m : cascades) {
		cascade_mats.push_back(m.view_proj_mat);
	}

	auto [bubo, stub] = ptc.create_scratch_buffer(vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer, std::span{cascade_mats});
	auto ubo = bubo;

	ptc.wait_all_transfers();

	for (u8 i = 0; i < SHADOW_MAP_CASCADE_COUNT; ++i) {
		const vuk::Resource layer_resource{m_attachment_names[i], vuk::Resource::Type::eImage, vuk::eDepthStencilRW};

		rg.add_pass(vuk::Pass{
			.resources = {layer_resource},
			.execute =
				[=](vuk::CommandBuffer& cbuf) {
					cbuf.set_viewport(0, vuk::Rect2D::absolute(0, 0, DIMENSION, DIMENSION))
						.set_scissor(0, vuk::Rect2D::absolute(0, 0, DIMENSION, DIMENSION))
						.set_primitive_topology(vuk::PrimitiveTopology::eTriangleList)
						.bind_graphics_pipeline("depth_only")
						.bind_uniform_buffer(0, 0, ubo)
						.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, static_cast<u32>(i));

					renderer.render(cbuf, [&](const MeshComponent& mesh, const vuk::Buffer& transform) {
						cbuf.bind_uniform_buffer(1, 0, transform);
						return vuk::Packed{vuk::Format::eR32G32B32Sfloat, vuk::Format::eR32G32B32Sfloat, vuk::Ignore{vuk::Format::eR32G32Sfloat}};
					});
				},
		});

		rg.attach_image(m_attachment_names[i],
			vuk::ImageAttachment{
				.image = *m_shadow_map.image,
				.image_view = *m_image_views[i],
				.extent = vuk::Extent2D{DIMENSION, DIMENSION},
				.format = vuk::Format::eD32Sfloat,
				.sample_count = vuk::Samples::e1,
				.clear_value = vuk::ClearDepthStencil{1.f, 0},
			},
			vuk::Access::eClear, vuk::Access::eFragmentSampled);
	}
}

vuk::ImageView CascadedShadowRenderPass::shadow_map_view() const {
	return *m_shadow_map_view;
}

// https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp
std::array<CascadedShadowRenderPass::CascadeInfo, CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT> CascadedShadowRenderPass::compute_cascades(
	const RenderInfo& info) {
	std::array<CascadeInfo, SHADOW_MAP_CASCADE_COUNT> cascades;

	f32 cascade_splits[SHADOW_MAP_CASCADE_COUNT];

	f32 near_clip = info.cam_proj.near;
	f32 far_clip = info.cam_proj.far;
	f32 clip_range = far_clip - near_clip;

	f32 min_z = near_clip;
	f32 max_z = near_clip + clip_range;

	f32 range = max_z - min_z;
	f32 ratio = max_z / min_z;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (u8 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		f32 p = (i + 1) / static_cast<f32>(SHADOW_MAP_CASCADE_COUNT);
		f32 log = min_z * std::pow(ratio, p);
		f32 uniform = min_z + range * p;
		f32 d = cascade_split_lambda * (log - uniform) + uniform;
		cascade_splits[i] = (d - near_clip) / clip_range;
	}

	// Calculate orthographic projection matrix for each cascade
	f32 last_split_dist = 0.0;
	for (u8 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		f32 split_dist = cascade_splits[i];

		glm::vec3 frustum_corners[8] = {
			glm::vec3(-1.0f, 1.0f, -1.0f),
			glm::vec3(1.0f, 1.0f, -1.0f),
			glm::vec3(1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, -1.0f, 1.0f),
			glm::vec3(-1.0f, -1.0f, 1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 inv_cam = glm::inverse(info.cam_proj.matrix() * info.cam_view);
		for (u8 i = 0; i < 8; i++) {
			glm::vec4 inv_corner = inv_cam * glm::vec4(frustum_corners[i], 1.0f);
			frustum_corners[i] = inv_corner / inv_corner.w;
		}

		for (u8 i = 0; i < 4; i++) {
			glm::vec3 dist = frustum_corners[i + 4] - frustum_corners[i];
			frustum_corners[i + 4] = frustum_corners[i] + (dist * split_dist);
			frustum_corners[i] = frustum_corners[i] + (dist * last_split_dist);
		}

		// Get frustum center
		glm::vec3 frustum_center = glm::vec3(0.0f);
		for (u8 i = 0; i < 8; i++) {
			frustum_center += frustum_corners[i];
		}
		frustum_center /= 8.0f;

		f32 radius = 0.0f;
		for (u8 i = 0; i < 8; i++) {
			f32 distance = glm::length(frustum_corners[i] - frustum_center);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 max_extents = glm::vec3(radius);
		glm::vec3 min_extents = -max_extents;

		glm::mat4 light_view_matrix = glm::lookAt(frustum_center - info.light_direction * -min_extents.z, frustum_center, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 light_ortho_matrix = glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);

		// Store split distance and matrix in cascade
		cascades[i].split_depth = (info.cam_proj.near + split_dist * clip_range) * -1.0f;
		cascades[i].view_proj_mat = light_ortho_matrix * light_view_matrix;

		last_split_dist = cascade_splits[i];
	}

	return cascades;
}
