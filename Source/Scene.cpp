#include "Scene.hpp"

#include "Context.hpp"
#include "GfxUtil.hpp"

#include <vuk/Context.hpp>
#include <vuk/CommandBuffer.hpp>

SceneRenderer SceneRenderer::create(Context& ctxt, Scene& scene) {
	static constexpr u32 MAX_SCENE_OBJECTS = 1000;

	SceneRenderer sr;

	sr.m_ctxt = &ctxt;
	sr.m_scene = &scene;
	sr.m_transform_buffer = ctxt.vuk_context->allocate_buffer(
		vuk::MemoryUsage::eCPUtoGPU, vuk::BufferUsageFlagBits::eUniformBuffer | vuk::BufferUsageFlagBits::eTransferDst,
		MAX_SCENE_OBJECTS * gfx_util::uniform_buffer_offset_alignment<glm::mat4>(ctxt), gfx_util::uniform_buffer_offset_alignment<glm::mat4>(ctxt));

	return sr;
}

void SceneRenderer::update(vuk::PerThreadContext& ptc, Scene& scene, const entt::view<entt::exclude_t<>, MeshComponent, TransformComponent>& scene_view) {
	m_scene = &scene;

	m_cached_meshes.clear();
	m_cached_meshes.reserve(scene_view.size_hint());

	u64 offset = 0;
	scene_view.each([&](const MeshComponent& mesh, const TransformComponent& transform) {
		m_cached_meshes.push_back(mesh);
		ptc.upload(m_transform_buffer.subrange(offset, sizeof(glm::mat4)), std::span{&transform.matrix, 1});
		offset += gfx_util::uniform_buffer_offset_alignment<glm::mat4>(*m_ctxt);
	});

	ptc.wait_all_transfers();
}

void SceneRenderer::render(vuk::CommandBuffer& out_cbuf, std::function<vuk::Packed(const MeshComponent&, const vuk::Buffer&)> binder) const {
	u64 offset = 0;
	for (const auto& mesh : m_cached_meshes) {
		auto packed = binder(mesh, vuk::Buffer{m_transform_buffer}.subrange(offset, sizeof(glm::mat4)));
		out_cbuf.bind_vertex_buffer(0, *m_scene->meshes.get(mesh.mesh).verts, 0, packed)
			.bind_index_buffer(*m_scene->meshes.get(mesh.mesh).inds, vuk::IndexType::eUint32);
		out_cbuf.draw_indexed(m_scene->meshes.get(mesh.mesh).mesh.second.size(), 1, 0, 0, 0);
		offset += gfx_util::uniform_buffer_offset_alignment<glm::mat4>(*m_ctxt);
	}
}

Scene& SceneRenderer::scene() {
	return *m_scene;
}

const Scene& SceneRenderer::scene() const {
	return *m_scene;
}
