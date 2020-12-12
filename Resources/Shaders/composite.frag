#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D base;
layout (set = 0, binding = 1) uniform sampler2D volumetric_light;

const vec3 VOLUMETRIC_LIGHT_COLOR = vec3(1, 0.9, 0.7);

void main() {
	out_color = texture(base, in_uv);
	
	// composite volumetric light
	vec4 volumetric_light = texture(volumetric_light, in_uv);
	out_color.rgb = mix(out_color.rgb, volumetric_light.rrr * VOLUMETRIC_LIGHT_COLOR, volumetric_light.r * 0.002);
}
