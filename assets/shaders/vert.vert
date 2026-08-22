#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 frag_color;

layout(push_constant) uniform PushBlock {
    mat4 mvp;
} push;

void main() {
    gl_Position = push.mvp * vec4(position, 1.0);
    frag_color = color;
}
