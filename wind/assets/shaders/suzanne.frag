#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec2 frag_uv;
layout(location = 3) in vec4 frag_tangent;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    uint albedo_texture;
    uint normal_index;
    uint metallic_roughness_index;
    vec4 base_color;
} pc;

void main()
{
    vec3 normal =
        normalize(frag_normal);

    if (pc.normal_index != 0xFFFFFFFF)
    {
        vec3 tangent =
            normalize(frag_tangent.xyz);

        tangent =
            normalize(
                tangent -
                    normal * dot(normal, tangent)
            );

        vec3 bitangent =
            normalize(
                cross(normal, tangent)
            ) * frag_tangent.w;

        mat3 TBN =
            mat3(
                tangent,
                bitangent,
                normal
            );

        vec2 xy =
            texture(
                textures[pc.normal_index],
                frag_uv
            ).rg;

        xy =
            xy * 2.0 - 1.0;

        float z =
            sqrt(
                max(
                    1.0 - dot(xy, xy),
                    0.0
                )
            );

        vec3 tangent_normal =
            vec3(
                xy,
                z
            );

        normal =
            normalize(
                TBN * tangent_normal
            );
    }

    vec3 light_direction =
        normalize(
            vec3(
                -0.4,
                -0.6,
                -1.0
            )
        );

    float diffuse =
        max(
            dot(normal, light_direction),
            0.0
        );

    vec3 albedo;

    if (pc.albedo_texture == 0xFFFFFFFF)
    {
        albedo =
            pc.base_color.rgb;
    }
    else
    {
        albedo =
            texture(
                textures[pc.albedo_texture],
                frag_uv
            ).rgb;
    }

    float ambient = 0.18;

    vec3 color =
        albedo *
            (
            ambient +
                diffuse * 0.82
            );

    out_color =
        vec4(
            color,
            1.0
        );
}
