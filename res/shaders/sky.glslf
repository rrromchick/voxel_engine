#version 330 core

in vec3 a_ray_dir;
out vec4 f_color;

uniform float u_time;
uniform vec3 u_sun_dir;

void main() {
    vec3 ray = normalize(a_ray_dir);

    float sun_height = u_sun_dir.y;

    vec3 sky_day_top = vec3(0.2, 0.5, 0.9);
    vec3 sky_day_hor = vec3(0.5, 0.7, 1.0);
    vec3 sky_night_top = vec3(0.02, 0.03, 0.05);
    vec3 sky_night_hor = vec3(0.05, 0.05, 0.1);
    vec3 sky_sunset = vec3(0.9, 0.35, 0.15);

    float day_factor = clamp(sun_height * 2.0, 0.0, 1.0);
    float sunset_factor = clamp(1.0 - abs(sun_height * 4.0), 0.0, 1.0);

    vec3 top_color = mix(sky_night_top, sky_day_top, day_factor);
    vec3 hor_color = mix(sky_night_hor, sky_day_hor, day_factor);

    hor_color = mix(hor_color, sky_sunset, sunset_factor * clamp(sun_height + 0.2, 0.0, 1.0));

    float horizon_mix = pow(1.0 - max(ray.y, 0.0), 4.0);
    vec3 final_sky = mix(top_color, hor_color, horizon_mix);

    float sun_dot = max(dot(ray, u_sun_dir), 0.0);
    if (sun_dot > 0.998) {
        final_sky = mix(final_sky, vec3(1.0, 1.0, 0.9), clamp((sun_dot - 0.998) * 500.0, 0.0, 1.0));
    } else if (sun_dot > 0.99) {
        float glow = (sun_dot - 0.99) / (0.998 - 0.99);
        final_sky += vec3(0.9, 0.6, 0.3) * glow * 0.5 * day_factor;
    }

    vec3 moon_dir = -u_sun_dir;
    float moon_dot = max(dot(ray, moon_dir), 0.0);
    if (moon_dot > 0.9985) {
        final_sky = mix(final_sky, vec3(0.9, 0.9, 0.95), clamp((moon_dot - 0.9985) * 600.0, 0.0, 1.0));
    }

    f_color = vec4(final_sky, 1.0);
}
