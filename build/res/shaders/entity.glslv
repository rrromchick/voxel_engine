#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_tex_coord;
layout (location = 2) in vec4 v_color;

out vec4 a_color;
out vec2 a_tex_coord;

uniform mat4 u_model;
uniform mat4 u_projview;
uniform vec3 u_sky_light_color;
uniform float u_gamma;

uniform vec3 u_env_block_light;
uniform float u_env_sky_light;

void main() {
    vec4 position = u_projview * u_model * vec4(v_position, 1.0);

    vec3 block_light = u_env_block_light;
    vec3 sky_light = u_sky_light_color * u_env_sky_light;
    vec3 ambient_floor = vec3(0.05);

    vec3 combined_light = block_light + sky_light + ambient_floor;
    combined_light = min(combined_light, vec3(1.0));

    vec3 shaded_color = v_color.rgb * combined_light;

    a_color = vec4(pow(shaded_color, vec3(1.0 / u_gamma)), v_color.a);
    a_tex_coord = v_tex_coord;
    gl_Position = position;
}