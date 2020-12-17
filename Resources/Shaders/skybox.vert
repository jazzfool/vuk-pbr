#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_uv;

layout(set = 0, binding = 0) uniform Uniforms {
	mat4 projection;
	mat4 view;
};

layout(set = 0, binding = 1) uniform Model {
	mat4 model;
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	out_uv = in_pos;
	out_uv.y *= -1;

	gl_Position = projection * view * model * vec4(in_pos, 1);
}
