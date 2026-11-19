#version 330 core

layout (location = 0) in vec3 v_position;

out vec3 a_ray_dir;

uniform mat4 u_view;
uniform mat4 u_proj;

void main() {
    a_ray_dir = v_position;

    mat4 static_view = mat4(mat3(u_view));
    vec4 pos = u_proj * static_view * vec4(v_position, 1.0);

    gl_Position = pos.xyzw;
}
