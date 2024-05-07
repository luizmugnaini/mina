#version 460

layout(location = 0) in vec3  frag_col;
layout(location = 0) out vec4 res_col;

void main() {
    res_col = vec4(frag_col, 1.0);
}
