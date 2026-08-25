#version 450

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 frag_position;

layout(push_constant) uniform PushConstants
{
    mat4 transform;
} pc;

void main()
{
    vec4 position = pc.transform * vec4(in_position, 1.0);

    frag_position = position.xyz;

    gl_Position = position;
}
