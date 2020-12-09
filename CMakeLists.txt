cmake_minimum_required(VERSION 3.10)

project(vukpbr CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE)

set(Source

    Source/main.cpp
    Source/Resource.cpp
    Source/Mesh.cpp
    Source/GfxUtil.cpp
    Source/STB.cpp
    Source/Context.cpp
    Source/Renderer.cpp
    Source/Scene.cpp
    Source/Frustum.cpp
    
    Source/GfxParts/CascadedShadows.cpp
)

set(Resources
    
    Resources/Shaders/pbr.vert
    Resources/Shaders/pbr.frag
    Resources/Shaders/cubemap.vert
    Resources/Shaders/equirectangular_to_cubemap.frag
    Resources/Shaders/irradiance_convolution.frag
    Resources/Shaders/prefilter.frag
    Resources/Shaders/brdf.vert
    Resources/Shaders/brdf.frag
    Resources/Shaders/depth_only.vert
    Resources/Shaders/depth_only.frag
    Resources/Shaders/debug_shadow_map.vert
    Resources/Shaders/debug_shadow_map.frag

    Resources/Textures/rust_albedo.jpg
    Resources/Textures/rust_metallic.png
    Resources/Textures/rust_roughness.jpg
    Resources/Textures/rust_normal.jpg
    Resources/Textures/rust_ao.jpg

    Resources/Textures/forest_slope_1k.hdr

    Resources/Meshes/Sphere.obj
)

add_executable(vukpbr ${Source})

include(CMakeRC.cmake)

cmrc_add_resource_library(VPBRResources ALIAS VPBR::Resources NAMESPACE vpbr ${Resources})

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)

add_subdirectory(ThirdParty/vuk)
add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/spdlog)
add_subdirectory(ThirdParty/vk-bootstrap)
add_subdirectory(ThirdParty/glm)
add_subdirectory(ThirdParty/entt)
add_subdirectory(ThirdParty/tinyobjloader)

target_link_libraries(vukpbr PRIVATE VPBR::Resources vuk glfw spdlog vk-bootstrap glm EnTT tinyobjloader)
target_include_directories(vukpbr PRIVATE ThirdParty/stb)
