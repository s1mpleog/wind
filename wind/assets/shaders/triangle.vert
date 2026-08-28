#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
    uint albedo_texture;
    uint normal_index;
    uint metallic_roughness_index;
    vec4 base_color;
} pc;

// layout(set = 0, binding = 1) uniform UboInstance {
//     mat4 transform
// } ubo_instance;

void main()
{
    vec4 position = pc.transform * vec4(in_position, 1.0);

    gl_Position = position;

    out_color = vec4(in_color, 1.0f);
}
