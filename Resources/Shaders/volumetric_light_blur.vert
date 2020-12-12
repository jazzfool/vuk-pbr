#version 450
#pragma shader_stage(vertex)

layout (location = 0) out vec2 out_uv;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    // fullscreen triangle
    out_uv = vec2(0);
    out_uv.x = (gl_VertexIndex == 2) ? 2.0 : 0.0;
    out_uv.y = (gl_VertexIndex == 1) ? 2.0 : 0.0;
    gl_Position = vec4(out_uv * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 1.0, 1.0);
}
