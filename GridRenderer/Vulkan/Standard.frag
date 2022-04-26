#version 450

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 3) uniform sampler samp;
layout(binding = 0, set = 4) uniform texture2D diffuseTexture;
layout(binding = 0, set = 5) uniform texture2D specularTexture;

struct Light
{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

// This buffer is small, but uniform buffers don't allow unsized arrays so an
// ssbo has to be used
layout(binding = 0, set = 6) readonly buffer LightBuffer
{
    Light lights[];
}
lights;

// The interface name is optional
layout(binding = 0, set = 7) uniform CameraBuffer
{
    vec3 cameraPos;
};

void main()
{
    vec3 ambient = vec3(0.1, 0.1, 0.1);
    float shininessFactor = 5.0;
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    uint nrOfLights = lights.lights.length();

    vec3 diffuseMaterial = texture(sampler2D(diffuseTexture, samp), uv).xyz;
    vec3 specularMaterial = texture(sampler2D(specularTexture, samp), uv).xyz;

    vec3 position = worldPosition;
    vec3 normal = normalize(normal);
    vec3 viewVector = normalize(cameraPos - position);

    for(uint i = 0; i < nrOfLights; ++i)
    {
        vec3 lightPosition = vec3(lights.lights[i].x, lights.lights[i].y, lights.lights[i].z);
        vec3 lightColour = vec3(lights.lights[i].r, lights.lights[i].g, lights.lights[i].b);

        vec3 lightVector = normalize(lightPosition - position);
        float cosA = max(0.0, dot(lightVector, normal));
        diffuse += lightColour * cosA;

        vec3 reflection = reflect(-lightVector, normal);
        specular += lightColour * max(0.0, pow(dot(reflection, viewVector), shininessFactor));
    }

    vec3 illumination = (ambient + diffuse) * diffuseMaterial + specularMaterial * specular;

    outColor = vec4(illumination, 1.0);
}