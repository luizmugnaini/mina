#version 460

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec3 vtx_col;

layout(location = 0) out vec3 frag_col;

void main() {
    gl_Position = vec4(vtx_pos, 1.0);
    frag_col    = vtx_col;
}
