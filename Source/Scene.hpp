#pragma once

#include "Mesh.hpp"

#include <entt/entt.hpp>
#include <functional>

namespace vuk {
class CommandBuffer;
class PerThreadContext;
struct Packed;
} // namespace vuk

class Scene {
  public:
	entt::registry registry;

	MeshCache meshes;
	TextureCache textures;

  private:
};

class SceneRenderer {
  public:
	static SceneRenderer create(struct Context& ctxt, Scene& scene);

	void update(vuk::PerThreadContext& ptc, Scene& scene, const entt::view<entt::exclude_t<>, MeshComponent, TransformComponent>& scene_view);
	void render(vuk::CommandBuffer& out_cbuf, std::function<vuk::Packed(const MeshComponent&, const vuk::Buffer&)> binder) const;

	Scene& scene();
	const Scene& scene() const;

  private:
	struct Context* m_ctxt;
	Scene* m_scene;

	std::vector<MeshComponent> m_cached_meshes;
	vuk::Buffer m_transform_buffer;
};

void pbr_binder(vuk::CommandBuffer&, const MeshComponent&, Scene&);
