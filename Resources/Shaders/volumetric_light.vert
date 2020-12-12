#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec3 out_ray_dir;
layout (location = 1) out vec2 out_uv;

layout (set = 0, binding = 0) uniform Camera {
    mat4 inv_view_proj;
    vec2 clip_range;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    out_ray_dir = (inv_view_proj * vec4(in_pos.xy * (clip_range.y - clip_range.x), clip_range.y + clip_range.x, clip_range.y - clip_range.x)).xyz;
    out_uv = in_uv;

    gl_Position = vec4(in_pos.xy * vec2(1, -1), 0, 1);
}
