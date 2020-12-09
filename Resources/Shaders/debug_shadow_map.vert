#version 450
#pragma shader_stage(vertex)

layout(push_constant) uniform PushConsts {
    uint cascade_index;
};

layout (location = 0) out vec2 out_uv;
layout (location = 1) out uint out_cascade_index;

void main() {
    out_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    out_cascade_index = cascade_index;
    gl_Position = vec4(out_uv * 2.f - 1.f, 0.f, 1.f);
}
