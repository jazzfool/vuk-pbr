#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

void main()
{
    out_uv = in_uv;
    gl_Position = vec4(in_pos, 1.0);
}