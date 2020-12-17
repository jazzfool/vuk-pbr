#pragma once

#include "../Types.hpp"
#include "../Perspective.hpp"

#include <vuk/Image.hpp>
#include <glm/mat4x4.hpp>

namespace vuk {
class PerThreadContext;
struct RenderGraph;
class CommandBuffer;
struct Buffer;
} // namespace vuk

class AtmosphericSkyCubemap {
  public:
	static glm::mat4 skybox_model_matrix(const Perspective& cam_proj, glm::vec3 cam_pos);

	AtmosphericSkyCubemap(glm::vec3 light_direction);

	void init(vuk::PerThreadContext& ptc, struct Context& ctxt, class PipelineStore& ps, const struct RenderMesh& cube);
	void draw(vuk::CommandBuffer& cbuf, const vuk::Buffer& ubo, const struct RenderMesh& cube);

	Perspective cam_proj;
	glm::vec3 cam_pos;

  private:
	const glm::vec3 m_light_direction;
	vuk::Texture m_cubemap;
	vuk::Unique<vuk::ImageView> m_cubemap_view;
};
