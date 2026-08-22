#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 ourColor; 

void main() {
    gl_Position = vec4(inPosition, 1.0);
    ourColor = inColor;
}
