#version 450
#pragma shader_stage(vertex)

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_pos;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec3 out_mv_pos;

layout(set = 0, binding = 0) uniform Uniforms {
	mat4 projection;
	mat4 view;
};

layout(set = 1, binding = 0) uniform Object {
	mat4 model;
};

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	out_uv = in_uv * 2;
	vec4 locPos = model * vec4(in_pos, 1.0);
	out_pos = locPos.xyz / locPos.w;
	out_normal = normalize(transpose(inverse(mat3(model))) * in_normal);
	out_mv_pos = (view * vec4(out_pos, 1.0)).xyz;

	gl_Position = projection * view * vec4(out_pos, 1.0);
}