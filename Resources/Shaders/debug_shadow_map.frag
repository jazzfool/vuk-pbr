#version 450
#pragma shader_stage(fragment)

layout(binding = 0) uniform sampler2DArray shadow_map;

layout(location = 0) in vec2 in_uv;
layout(location = 1) flat in uint in_cascade_index;

layout(location = 0) out vec4 out_frag_color;

void main() {
	float depth = texture(shadow_map, vec3(in_uv, float(in_cascade_index))).r;
	out_frag_color = vec4(vec3(depth), 1.0);
}
