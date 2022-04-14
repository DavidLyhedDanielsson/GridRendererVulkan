#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec2 outUv;
layout(location = 2) out vec3 outNormal;

layout(binding = 0) uniform TransformBuffer {
    mat4 worldMatrix;
} transformBuffer;
layout(binding = 1) uniform CameraBuffer {
    mat4 viewProjMatrix;
} cameraBuffer;

void main() {
    vec4 worldPosition = transformBuffer.worldMatrix * vec4(position, 1.0);
    gl_Position = cameraBuffer.viewProjMatrix * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outUv = uv;
    outNormal = normal;
}