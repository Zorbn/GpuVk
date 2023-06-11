#version 450

layout(binding = 1) uniform sampler2DArray texSampler;
layout(binding = 2) uniform sampler2D depthSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = vec4(0.0, 1.0, 0.0, 1.0);
    outColor = texture(depthSampler, fragTexCoord.xy);
}