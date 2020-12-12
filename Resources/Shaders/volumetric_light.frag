#version 450
#pragma shader_stage(fragment)

#define SHADOW_MAP_CASCADE_COUNT 4
#define PI 3.14159265358979323

layout (location = 0) in vec3 in_ray_dir;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_fog;

layout (set = 0, binding = 0) uniform Camera {
    mat4 inv_view_proj;
    vec2 clip_range;
};

layout (set = 0, binding = 1) uniform sampler2D g_position;
layout (set = 0, binding = 2) uniform sampler2DArray shadow_map;
layout (set = 0, binding = 3) uniform sampler2D depth;

layout (set = 0, binding = 4) uniform Uniforms {
    vec4 cascade_splits;
    mat4 cascade_view_proj_mats[SHADOW_MAP_CASCADE_COUNT];
	mat4 inv_view;
    vec3 light_direction;
	float _pad;
	vec3 cam_pos;
};

const int num_steps = 10;
const float scattering = 0.2;

const float light_intensity = 1000;
const vec3 light_color = vec3(1, 1, 1);

float compute_scattering(float light_dot_view) {
	float result = 1 - scattering * scattering;
	result /= (4 * PI * pow(1 + scattering * scattering - (2 * scattering) * light_dot_view, 1));
	return result;
}

float texture_proj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
    float bias = 0.005;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
		float dist = texture(shadow_map, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = 0; // ambient light
		}
	}

	return shadow;
}

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

// shadows :)
float shadow_calculation(vec3 mv_pos, vec3 pos) {
    uint cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if (mv_pos.z < cascade_splits[i]) {
            cascadeIndex = i + 1;
        }
    }

    vec4 sc = (biasMat * cascade_view_proj_mats[cascadeIndex]) * vec4(pos, 1.0);
    sc = sc / sc.w;

	/*ivec2 texDim = textureSize(shadow_map, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 8;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += texture_proj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}

	return shadowFactor / count;
	*/

	return texture_proj(sc, vec2(0), cascadeIndex);
}

const float dither_pattern[4][4] = {
	{0.0f, 0.5f, 0.125f, 0.625f},
	{0.75f, 0.22f, 0.875f, 0.375f},
	{0.1875f, 0.6875f, 0.0625f, 0.5625},
	{0.9375f, 0.4375f, 0.8125f, 0.3125}
};

// this is raymarched volumetric light scattering... not ideal for perf
void old_main() {
	vec3 mv_pos = texture(g_position, in_uv).xyz / texture(g_position, in_uv).w;

	vec4 world_pos_inv = inv_view * vec4(mv_pos, 1);

	vec3 world_pos = world_pos_inv.xyz / world_pos_inv.w;
	vec3 start_pos = cam_pos;

	vec3 ray_vector = world_pos - start_pos;
	float ray_length = length(ray_vector);
	vec3 ray_dir = normalize(ray_vector);
	float step_length = ray_length / num_steps;
	vec3 step = ray_dir * step_length;

	const float dither_value = dither_pattern[int(in_uv.x * 800) % 4][int(in_uv.y * 600) % 4];
	start_pos += step * dither_value;

	vec3 current_pos = start_pos;

	out_fog = vec4(0);
	for (int i = 0; i < num_steps; ++i) {
		vec4 current_mv_pos = inverse(inv_view) * vec4(current_pos, 1);
		current_mv_pos /= current_mv_pos.w;

		float dist = length(cam_pos - current_pos);
		float fade = dist > 20 ? 1 : dist / 20;

		if (shadow_calculation(current_mv_pos.xyz, current_pos) > 0.5) {
			out_fog += vec4(compute_scattering(dot(ray_dir, light_direction))) * vec4(light_color, 1) * light_intensity * fade;
		}

		current_pos += step;
	}

	out_fog /= num_steps;
}

// simpler and better
void main() {
	const int NUM_STEPS = 32;
	const float NUM_DELTA = 1.0 / NUM_STEPS;
	const float MAX_DELTA_LEN = 10;
	const float INIT_DECAY = 1;
	const float DIST_DECAY = 3;

	vec4 light_pos = inverse(inv_view_proj) * vec4(-light_direction * clip_range.y, 1);
	light_pos /= light_pos.w;

	vec2 sun_uv = (light_pos.xy + vec2(1)) / vec2(2);

	// screen-space coords
	vec2 dir_to_sun = sun_uv - in_uv;
	float length_to_sun = length(dir_to_sun);
	dir_to_sun = normalize(dir_to_sun);

	float delta_len = min(MAX_DELTA_LEN, length_to_sun * NUM_DELTA);
	vec2 ray_delta = dir_to_sun * delta_len;

	float step_decay = DIST_DECAY * delta_len;

	vec2 ray_offset = vec2(0);
	float decay = INIT_DECAY;
	float ray_intensity = 0;
	
	const float dither_value = dither_pattern[int(in_uv.x * 800) % 4][int(in_uv.y * 600) % 4];
	ray_offset += dither_value * ray_delta;

	for (int i = 0; i < NUM_STEPS; ++i) {
		vec2 samp_pos = in_uv + ray_offset;
		ray_intensity += float(texture(depth, samp_pos).r == 1) * decay;
		ray_offset += ray_delta;
		decay = clamp(decay - step_decay, 0, 1);
	}

	out_fog = vec4(ray_intensity);
}
