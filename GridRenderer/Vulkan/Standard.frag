#version 450

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 3) uniform sampler samp;
layout(binding = 0, set = 4) uniform texture2D tex;

void main()
{
    outColor = vec4(texture(sampler2D(tex, samp), uv).xyz, 1.0);
}