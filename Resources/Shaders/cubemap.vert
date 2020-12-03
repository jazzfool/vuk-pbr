#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 WorldPos;

layout (binding = 0) uniform Projection {
    mat4 projection;
};
layout (binding = 1) uniform View {
    mat4 view;
};

void main()
{
    WorldPos = aPos;  
    gl_Position =  projection * view * vec4(WorldPos, 1.0);
}