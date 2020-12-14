#pragma once

#include "Types.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"
#include "Material.hpp"
#include "GfxParts/CascadedShadows.hpp"
#include "GfxParts/SSAO.hpp"
#include "GfxParts/GBuffer.hpp"
#include "GfxParts/VolumetricLights.hpp"
#include "GfxParts/Atmosphere.hpp"

#include <glm/vec3.hpp>
#include <vuk/Image.hpp>
#include <vuk/RenderGraph.hpp>
#include <optional>

class Renderer {
  public:
	void init(struct Context& ctxt);

	void update();
	void render();

	void mouse_event(f64 x_pos, f64 y_pos);

  private:
	vuk::RenderGraph render_graph(vuk::PerThreadContext& ptc);

	struct Context* m_ctxt;

	Scene m_scene;
	SceneRenderer m_scene_renderer;

	CascadedShadowRenderPass m_cascaded_shadows;
	SSAODepthPass m_ssao;
	GBufferPass m_gbuffer;
	VolumetricLightPass m_volumetric_light;
	AtmosphericSkyCubemap m_atmosphere;

	f64 m_last_x;
	f64 m_last_y;
	f32 m_pitch;
	f32 m_yaw;
	glm::vec3 m_cam_pos;
	glm::vec3 m_cam_front;
	glm::vec3 m_cam_up;

	vuk::Buffer m_transform_buffer;
	u32 m_transform_buffer_alignment;

	Mesh m_sphere;
	Mesh m_cube;
	Mesh m_quad;

	vuk::Texture m_hdr_texture;

	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_env_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_irradiance_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_prefilter_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_brdf_lut;
	vuk::Unique<vuk::ImageView> m_env_cubemap_iv;
	vuk::Unique<vuk::ImageView> m_irradiance_cubemap_iv;
	vuk::Unique<vuk::ImageView> m_prefilter_cubemap_iv;
};

struct RenderInfo {
	glm::vec3 light_direction;
	Perspective cam_proj;
	glm::mat4 cam_view;
	glm::vec3 cam_pos;
	glm::vec3 cam_forward;
	glm::vec3 cam_up;
	vuk::ImageView shadow_map;
	std::array<CascadedShadowRenderPass::CascadeInfo, CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT> cascades;
};
