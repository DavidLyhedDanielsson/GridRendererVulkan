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
layout(binding = 0, set = 1) uniform TransformBuffer
{
    mat4 worldMatrix;
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

    vec4 worldPosition = transpose(transformBuffer.worldMatrix)
                         * vec4(vertex.positionX, vertex.positionY, vertex.positionZ, 1.0);
    gl_Position = cameraBuffer.viewProjMatrix * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outUv = vec2(vertex.uvX, vertex.uvY);
    outNormal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
}