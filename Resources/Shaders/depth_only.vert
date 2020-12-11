#version 450
#pragma shader_stage(vertex)

// keep this in sync with CascadedShadowRenderPass::SHADOW_MAP_CASCADE_COUNT
#define SHADOW_MAP_CASCADE_COUNT 4

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

layout(set = 0, binding = 0) uniform Uniforms {
    mat4[SHADOW_MAP_CASCADE_COUNT] light_space_mats;
};

layout(set = 1, binding = 0) uniform Model {
    mat4 model;
};

layout(push_constant) uniform CascadeIndex {
    int cascade_index;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec3 light_direction = normalize(vec3(0, -2, 1));
    vec3 pos = in_pos;
    //pos -= in_normal * 0.01;
    //pos.z += 0.005 * tan(acos(dot(in_normal, -light_direction)));
    gl_Position = light_space_mats[cascade_index] * model * vec4(pos, 1.0);
}
