#version 450
#pragma shader_stage(fragment)

layout(location = 0) in vec3 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 2) uniform samplerCube cubemap;

vec3 uncharted2_tonemap_partial(vec3 x) {
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 uncharted2_filmic(vec3 v) {
	float exposure_bias = 2.0f;
	vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

	vec3 W = vec3(11.2f);
	vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
	return curr * white_scale;
}

void main() {
	vec3 color = texture(cubemap, in_uv).rgb;

	// HDR tonemapping
	color = uncharted2_filmic(color);
	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	out_color = vec4(color, 1);
}
