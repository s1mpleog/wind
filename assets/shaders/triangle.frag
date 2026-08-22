#version 450

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec3 ourColor;

void main() {
    out_color = vec4(ourColor, 1.0);
}
