#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D debug_image;

void main() {
    out_color = vec4(texture(debug_image, in_uv).rgb, 1.0);
}
