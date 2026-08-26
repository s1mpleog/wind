// #version 450

// layout(location = 0) in vec3 frag_position;

// layout(location = 0) out vec4 out_color;

// void main()
// {
//     // Reconstruct a face normal from the geometry.
//     vec3 dx = dFdx(frag_position);
//     vec3 dy = dFdy(frag_position);

//     vec3 normal = normalize(cross(dx, dy));

//     // Direction FROM the surface toward the light.
//     vec3 light_direction =
//         normalize(vec3(-0.4, -0.6, -1.0));

//     float diffuse = max(dot(normal, light_direction), 0.0);

//     // Neutral Suzanne gray.
//     vec3 albedo = vec3(0.68);

//     // Small amount of ambient light so the unlit side isn't black.
//     float ambient = 0.18;

//     vec3 color = albedo * (ambient + diffuse * 0.82);

//     out_color = vec4(color, 1.0);
// }

// #version 450

// layout(location = 0) in vec3 frag_position;
// layout(location = 1) in vec3 frag_normal;
// layout(location = 2) in vec2 frag_uv;

// layout(location = 0) out vec4 out_color;

// void main()
// {
//     // Reconstruct a face normal from the geometry.
//     vec3 dx = dFdx(frag_position);
//     vec3 dy = dFdy(frag_position);

//     vec3 face_normal = normalize(cross(dx, dy));

//     // Direction FROM the surface toward the light.
//     vec3 light_direction =
//         normalize(vec3(-0.4, -0.6, -1.0));

//     float diffuse = max(dot(face_normal, light_direction), 0.0);

//     // Your original neutral gray.
//     vec3 base_color = vec3(0.68);

//     // Visualize interpolated vertex normals.
//     // [-1,1] -> [0,1]
//     vec3 normal_color =
//         normalize(frag_normal) * 0.5 + 0.5;

//     // Visualize UVs.
//     // U -> red
//     // V -> green
//     vec3 uv_color = vec3(
//             fract(frag_uv.x),
//             fract(frag_uv.y),
//             0.0
//         );

//     // Subtle combination so the model remains mostly gray,
//     // while making UV/normal data visible.
//     vec3 debug_color = mix(
//             normal_color,
//             uv_color,
//             0.5
//         );

//     vec3 albedo =
//         base_color *
//             mix(vec3(1.0), debug_color, 0.35);

//     float ambient = 0.18;

//     vec3 color =
//         albedo *
//             (ambient + diffuse * 0.82);

//     out_color = vec4(color, 1.0);
// }

#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    uint albedo_texture;
} pc;

void main()
{
    vec3 normal = normalize(frag_normal);

    vec3 light_direction =
        normalize(vec3(-0.4, -0.6, -1.0));

    float diffuse = max(dot(normal, light_direction), 0.0);

    // Sample the bindless texture using the material's descriptor index.
    vec3 albedo = texture(
            textures[pc.albedo_texture],
            frag_uv
        ).rgb;

    float ambient = 0.18;

    vec3 color =
        albedo * (ambient + diffuse * 0.82);

    out_color = vec4(color, 1.0);
}
