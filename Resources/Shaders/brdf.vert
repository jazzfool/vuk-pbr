#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}