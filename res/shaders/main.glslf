#version 330 core

//in vec4 a_color;
out vec4 f_color;
in vec2 a_tex_coord;

uniform sampler2D u_texture0;

void main() {
    f_color = texture(u_texture0, a_tex_coord);
}