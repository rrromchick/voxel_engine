#version 330 core
in vec2 a_tex_coord;
in float a_light;
out vec4 f_color;
uniform sampler2D u_texture0;

void main() {   
    vec4 tex_color = texture(u_texture0, a_tex_coord);
    if (tex_color.a < 0.1) {
        discard;
    }
    f_color = vec4(tex_color.rgb * a_light, 1.0);
}