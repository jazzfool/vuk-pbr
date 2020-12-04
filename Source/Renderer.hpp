#pragma once

#include "Types.hpp"
#include "Mesh.hpp"

#include <glm/vec3.hpp>
#include <vuk/Image.hpp>
#include <vuk/RenderGraph.hpp>
#include <optional>

class Renderer {
  public:
	static std::optional<Renderer> create(struct Context& ctxt);

	void update();
	void render();

	void mouse_event(f64 x_pos, f64 y_pos);

  private:
	vuk::RenderGraph render_graph(vuk::PerThreadContext& ptc);

	Context* m_ctxt;

	f32 m_angle;
	f64 m_last_x;
	f64 m_last_y;
	f32 m_pitch;
	f32 m_yaw;
	glm::vec3 m_cam_pos;
	glm::vec3 m_cam_front;
	glm::vec3 m_cam_up;

	Mesh m_sphere;
	Mesh m_cube;
	Mesh m_quad;

	vuk::Texture m_hdr_texture;

	vuk::Texture m_albedo_texture;
	vuk::Texture m_metallic_texture;
	vuk::Texture m_normal_texture;
	vuk::Texture m_roughness_texture;
	vuk::Texture m_ao_texture;

	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_env_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_irradiance_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_prefilter_cubemap;
	std::pair<vuk::Texture, vuk::SamplerCreateInfo> m_brdf_lut;
	vuk::Unique<vuk::ImageView> m_env_cubemap_iv;
	vuk::Unique<vuk::ImageView> m_irradiance_cubemap_iv;
	vuk::Unique<vuk::ImageView> m_prefilter_cubemap_iv;
};
