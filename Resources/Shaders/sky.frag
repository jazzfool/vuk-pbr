#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform LightDirection {
	vec3 light_direction;
};

const float PI = 3.14159265359;

const int I_STEPS = 16;
const int J_STEPS = 8;

// Based on https://github.com/wwwtyro/glsl-atmosphere

vec2 rsi(vec3 r0, vec3 rd, float sr) {
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, r0);
	float c = dot(r0, r0) - (sr * sr);
	float d = (b * b) - 4.0 * a * c;

	if (d < 0.0)
		return vec2(1e5, -1e5);

	return vec2((-b - sqrt(d)) / (2.0 * a), (-b + sqrt(d)) / (2.0 * a));
}

vec3 atmosphere(vec3 r, vec3 r0, vec3 sun_pos, float sun_intensity, float planet_radius, float atmosphere_radius, vec3 rayleigh_coeff, float mie_coeff,
	float rayleigh_height, float mie_height, float g) {
	sun_pos = normalize(sun_pos);
	r = normalize(r);

	vec2 p = rsi(r0, r, atmosphere_radius);
	if (p.x > p.y)
		return vec3(0, 0, 0);
	p.y = min(p.y, rsi(r0, r, planet_radius).x);
	float i_step_size = (p.y - p.x) / float(I_STEPS);

	float i_time = 0.0;

	vec3 total_rlh = vec3(0, 0, 0);
	vec3 total_mie = vec3(0, 0, 0);

	float i_od_rlh = 0.0;
	float i_od_mie = 0.0;

	float mu = dot(r, sun_pos);
	float mumu = mu * mu;
	float gg = g * g;
	float rlh_pos = 3.0 / (16.0 * PI) * (1.0 + mumu);
	float mie_pos = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

	for (int i = 0; i < I_STEPS; i++) {
		vec3 i_pos = r0 + r * (i_time + i_step_size * 0.5);

		float i_height = length(i_pos) - planet_radius;

		float od_step_rlh = exp(-i_height / rayleigh_height) * i_step_size;
		float od_step_mie = exp(-i_height / mie_height) * i_step_size;

		i_od_rlh += od_step_rlh;
		i_od_mie += od_step_mie;

		float j_step_size = rsi(i_pos, sun_pos, atmosphere_radius).y / float(J_STEPS);

		float j_time = 0.0;

		float j_od_rlh = 0.0;
		float j_od_mie = 0.0;

		for (int j = 0; j < J_STEPS; j++) {
			vec3 j_pos = i_pos + sun_pos * (j_time + j_step_size * 0.5);

			float j_height = length(j_pos) - planet_radius;

			j_od_rlh += exp(-j_height / rayleigh_height) * j_step_size;
			j_od_mie += exp(-j_height / mie_height) * j_step_size;

			j_time += j_step_size;
		}

		vec3 attn = exp(-(mie_coeff * (i_od_mie + j_od_mie) + rayleigh_coeff * (i_od_rlh + j_od_rlh)));

		total_rlh += od_step_rlh * attn;
		total_mie += od_step_mie * attn;

		i_time += i_step_size;
	}

	return sun_intensity * (rlh_pos * rayleigh_coeff * total_rlh + mie_pos * mie_coeff * total_mie);
}

void main() {
	vec3 sun_pos = normalize(-light_direction);

	vec3 color = atmosphere(
		normalize(vec3(in_uv * 2 - vec2(1), -1)), vec3(0, 6372e3, 0), sun_pos, 22.0, 6371e3, 6471e3, vec3(5.5e-6, 13.0e-6, 22.4e-6), 21e-6, 8e3, 1.2e3, 0.758);

	color = 1 - exp(-1 * color);

	out_color = vec4(color, 1);
}
