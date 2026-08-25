#version 450

layout(location = 0) in vec3 frag_position;

layout(location = 0) out vec4 out_color;

void main()
{
    // Reconstruct a face normal from the geometry.
    vec3 dx = dFdx(frag_position);
    vec3 dy = dFdy(frag_position);

    vec3 normal = normalize(cross(dx, dy));

    // Direction FROM the surface toward the light.
    vec3 light_direction =
        normalize(vec3(-0.4, -0.6, -1.0));

    float diffuse = max(dot(normal, light_direction), 0.0);

    // Neutral Suzanne gray.
    vec3 albedo = vec3(0.68);

    // Small amount of ambient light so the unlit side isn't black.
    float ambient = 0.18;

    vec3 color = albedo * (ambient + diffuse * 0.82);

    out_color = vec4(color, 1.0);
}
