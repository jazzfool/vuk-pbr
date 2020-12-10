#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_tex_coords;

layout (set = 0, binding = 0) uniform Uniforms {
	mat4 proj;
	mat4 view;
};

layout (set = 1, binding = 0) uniform Model {
	mat4 model;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec4 view_pos = view * model * vec4(in_pos, 1);
	out_pos = view_pos.xyz;

	out_tex_coords = in_tex_coords;
	
	out_normal = normalize(transpose(inverse(mat3(model))) * in_normal);

    gl_Position = proj * view_pos;
}
