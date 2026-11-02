#version 330 core
out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D texture_diffuse1;

void main() {
	frag_color = texture(texture_diffuse1, tex_coords);
}