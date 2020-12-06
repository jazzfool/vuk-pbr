#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out vec2 TexCoords;
layout (location = 1) out vec3 WorldPos;
layout (location = 2) out vec3 Normal;

layout(set = 0, binding = 0) uniform Uniforms {
    mat4 projection;
    mat4 view;
};

layout(set = 1, binding = 0) uniform Object {
    mat4 model;
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    TexCoords = aTexCoords;
    vec4 locPos = model * vec4(aPos, 1.0);
    WorldPos = locPos.xyz / locPos.w;
    Normal = normalize(transpose(inverse(mat3(model))) * aNormal);

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}