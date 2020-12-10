#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 in_uv;

layout (location = 0) out float out_ao;

layout (set = 0, binding = 0) uniform sampler2D ssao;

void main()  {
    vec2 texel_size = 1.0 / vec2(textureSize(ssao, 0));
    float result = 0.0;

    for (int x = -2; x < 2; ++x)  {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(ssao, in_uv + offset).r;
        }
    }
	
    out_ao = result / (4.0 * 4.0);
}  