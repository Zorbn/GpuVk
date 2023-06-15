#version 450

precision highp float;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 instancePos;
layout(location = 4) in vec2 instanceSize;
layout(location = 5) in vec2 instanceTexPos;
layout(location = 6) in vec2 instanceTexSize;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition * vec3(instanceSize, 1.0) + instancePos, 1.0);
    fragColor = inColor;
    fragTexCoord = instanceTexPos + inTexCoord * instanceTexSize;
}
