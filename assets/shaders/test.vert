#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform Model {
    mat4 model;
    mat4 view;
    mat4 proj;
} model;

void main() {
    gl_Position = model.proj * model.view * model.model * vec4(inPosition, 1.0);

    fragColor = inColor;
}
