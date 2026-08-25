#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_mvp0;
layout(location = 3) in vec4 a_mvp1;
layout(location = 4) in vec4 a_mvp2;
layout(location = 5) in vec4 a_mvp3;
layout(location = 6) in vec4 a_uv_rect; // atlas uv positions

out vec2 uv;

void main() {
    mat4 mvp = mat4(a_mvp0, a_mvp1, a_mvp2, a_mvp3);
    uv = a_uv_rect.xy + a_uv * a_uv_rect.zw;
    gl_Position = mvp * vec4(a_pos, 1.0);
}
