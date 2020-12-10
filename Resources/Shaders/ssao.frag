#version 450
#pragma shader_stage(fragment)

#define KERNEL_SIZE 64

layout (location = 0) in vec2 in_uv;

layout (location = 0) out float out_ao;

layout (set = 0, binding = 0) uniform sampler2D g_position;
layout (set = 0, binding = 1) uniform sampler2D g_normal;
layout (set = 0, binding = 2) uniform sampler2D random_normal;
layout (set = 0, binding = 3) uniform Uniforms {
    vec4 samples[KERNEL_SIZE];
    mat4 projection;
};

const float radius = 0.5;
const float bias = 0.025;

layout (push_constant) uniform PushConsts {
    vec2 screen_size;
} push_consts;

void main() {
    vec2 noise_scale = vec2(push_consts.screen_size.x / 4, push_consts.screen_size.y / 4);

    vec3 frag_pos = texture(g_position, in_uv).xyz;
    vec3 normal = normalize(texture(g_normal, in_uv).rgb);
    vec3 random_vec = normalize(texture(random_normal, in_uv * noise_scale).xyz);

    vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        vec3 sample_pos = TBN * samples[i].xyz;
        sample_pos = frag_pos + sample_pos * radius;

        vec4 offset = vec4(sample_pos, 1);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sample_depth = texture(g_position, offset.xy).z;

        float range_check = smoothstep(0, 1, radius / abs(frag_pos.z - sample_depth));
        occlusion += float(sample_depth >= sample_pos.z + bias) * range_check;
    }

    occlusion = 1 - (occlusion / KERNEL_SIZE);

    out_ao = occlusion;
}
