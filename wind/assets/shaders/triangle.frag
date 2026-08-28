#version 450

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(1.0f, 1.0f, 0.5f, 1.0f);
}
