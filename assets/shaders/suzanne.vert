#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    uint albedo_texture;
    vec4 base_color;
} pc;

void main()
{
    vec4 position = pc.transform * vec4(in_position, 1.0);

    frag_position = position.xyz;
    frag_normal = normalize(mat3(pc.transform) * in_normal);
    frag_uv = in_uv;

    gl_Position = position;
}
