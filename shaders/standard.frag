#version 330 core

in vec3 v_normal;
in vec2 v_uv;

uniform vec4 u_color;
uniform sampler2D u_texture;

out vec4 frag_color;

const vec3 LIGHT_DIR = normalize(vec3(0.4, 1.0, 0.3)); // direction pointing TOWARD the light
const vec3 LIGHT_COLOR = vec3(1.0);
const vec3 AMBIENT = vec3(0.25);

void main() {
    vec3 n = normalize(v_normal);
    float diff = max(dot(n, LIGHT_DIR), 0.0);
    vec3 lighting = AMBIENT + LIGHT_COLOR * diff;

    vec4 base = texture(u_texture, v_uv) * u_color;
    frag_color = vec4(base.rgb * lighting, base.a);
}
