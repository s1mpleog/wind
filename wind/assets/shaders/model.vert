#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;
layout(location = 3) out vec4 frag_tangent;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    uint albedo_texture;
    uint normal_index;
    uint metallic_roughness_index;
    vec4 base_color;
} pc;

layout(set = 0, binding = 1) uniform UboInstance {
    mat4 transform;
} ubo_instance;

void main()
{
    vec4 position = pc.transform * vec4(in_position, 1.0);

    gl_Position = position;

    frag_position = position.xyz;

    mat3 normal_matrix = mat3(pc.transform);

    frag_normal = normalize(normal_matrix * in_normal);

    vec3 tangent = normalize(normal_matrix * in_tangent.xyz);

    frag_tangent = vec4(tangent, in_tangent.w);

    frag_uv = in_uv;
}
