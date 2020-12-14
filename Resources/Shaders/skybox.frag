#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 in_uv;

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 2) uniform samplerCube cubemap;

void main() {
	out_color = texture(cubemap, in_uv);
}
