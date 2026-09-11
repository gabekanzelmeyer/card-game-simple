#version 330 core

uniform vec4 u_color;
// uniform sampler2D u_texture;

out vec4 frag_color;

void main() {
    frag_color = u_color;
}
