// Vertex shader that uses vertex pulling
#version 460

// Input data has this format and no transformations are possible; vec3 can't be
// used since it would add padding
struct Vertex
{
    float positionX;
    float positionY;
    float positionZ;
    float uvX;
    float uvY;
    float normalX;
    float normalY;
    float normalZ;
};

// Should always be updated together
layout(binding = 0, set = 0) readonly buffer VertexBuffer
{
    Vertex vertices[];
}
vertexBuffer;
layout(binding = 1, set = 0) readonly buffer IndexBuffer
{
    uint indices[];
}
indexBuffer;

// Will be updated randomly
layout(binding = 0, set = 1) readonly buffer TransformBuffer
{
    mat4 worldMatrices[];
}
transformBuffer;

// Will be updated once per frame
layout(binding = 0, set = 2) uniform CameraBuffer
{
    mat4 viewProjMatrix;
}
cameraBuffer;

// Outputs
layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec2 outUv;
layout(location = 2) out vec3 outNormal;

void main()
{
    uint index = indexBuffer.indices[gl_VertexIndex];
    Vertex vertex = vertexBuffer.vertices[index];

    vec3 position = vec3(vertex.positionX, vertex.positionY, vertex.positionZ);
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    mat4 worldMatrix = transpose(transformBuffer.worldMatrices[gl_InstanceIndex]);

    vec4 worldPosition = worldMatrix * vec4(position, 1.0);
    gl_Position = cameraBuffer.viewProjMatrix * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outUv = vec2(vertex.uvX, vertex.uvY);
    outNormal = (worldMatrix * vec4(normal, 0.0)).xyz;
}