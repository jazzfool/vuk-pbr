#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

// MRT
layout (location = 0) out vec4 out_pos;
layout (location = 1) out vec4 out_normal;

layout (set = 1, binding = 1) uniform sampler2D normal_map;

vec3 get_normal_from_map()
{
    vec3 tangent_normal = texture(normal_map, in_tex_coords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(in_pos);
    vec3 Q2  = dFdy(in_pos);
    vec2 st1 = dFdx(in_tex_coords);
    vec2 st2 = dFdy(in_tex_coords);

    vec3 N   = normalize(in_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
}

void main() {
    out_pos = vec4(in_pos, 1.0);
	out_normal = vec4(get_normal_from_map(), 1.0);
}
