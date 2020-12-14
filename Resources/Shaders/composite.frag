#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D base;
layout (set = 0, binding = 1) uniform sampler2D volumetric_light;

const vec3 VOLUMETRIC_LIGHT_COLOR = vec3(1, 1, 0.9);

vec3 blend_soft_light(vec3 base, vec3 blend) {
    return mix(
        sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend), 
        2.0 * base * blend + base * base * (1.0 - 2.0 * blend), 
        step(base, vec3(0.5))
    );
}

void main() {
	vec3 color = texture(base, in_uv).rgb;

	// composite volumetric light
	float volumetric_light = texture(volumetric_light, in_uv).r;

    //color = mix(color, vec3(volumetric_light) * VOLUMETRIC_LIGHT_COLOR, volumetric_light * 0.002);
    color += vec3(volumetric_light) * VOLUMETRIC_LIGHT_COLOR * 0.2;

	out_color = vec4(color, 1);
}
