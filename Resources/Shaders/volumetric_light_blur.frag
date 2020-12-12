#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_volumetric_light;

layout (set = 0, binding = 0) uniform sampler2D volumetric_light;

void main()  {
    vec2 texel_size = 1.0 / vec2(textureSize(volumetric_light, 0));
    vec4 result = vec4(0);

    for (int x = -1; x < 1; ++x)  {
        for (int y = -1; y < 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(volumetric_light, in_uv + offset);
        }
    }
	
    out_volumetric_light = result / 4.0;
}