#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_tex_coord;
layout (location = 2) in vec4 v_light;

out vec4 a_color;
out vec2 a_tex_coord;

uniform mat4 u_model;
uniform mat4 u_projview;
uniform vec3 u_sky_light_color;
uniform float u_gamma;

void main() {
	vec4 position = u_projview * u_model * vec4(v_position, 1.0);
	vec3 block_light = v_light.rgb;
	vec3 sky_light = u_sky_light_color * v_light.a;

	vec3 ambient_floor = vec3(0.05);
	vec3 combined_light = block_light + sky_light + ambient_floor;

	combined_light = min(combined_light, vec3(1.0));
	a_color = vec4(pow(combined_light, vec3(1.0 / u_gamma)), 1.0);

	a_tex_coord = v_tex_coord;
	gl_Position = position;
}