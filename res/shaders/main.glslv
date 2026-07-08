#version 330 core

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec2 v_tex_coord;
//layout (location = 2) in float v_light;

//out vec4 a_color;
out vec2 a_tex_coord;

uniform mat4 model;
uniform mat4 projview;

void main() {
//    a_color = vec4(v_light, v_light, v_light, 1.0f);
    a_tex_coord = v_tex_coord;
    gl_Position = projview * model * vec4(v_position, 1.0);
}