#pragma once

#include "Mesh.hpp"

#include <entt/entt.hpp>

class Scene {
  public:
	entt::registry registry;

	MeshCache meshes;
	TextureCache textures;

  private:
};
