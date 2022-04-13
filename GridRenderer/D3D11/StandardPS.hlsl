struct PixelShaderInput
{
	float4 position : SV_POSITION;
	float4 worldPos : WORLD_POSITION;
	float2 uv : UV;
	float3 normal : NORMAL;
};

struct PointLight
{
	float3 position;
	float3 colour;
};

Texture2D diffuseTexture : register(t0);
Texture2D specularTexture : register(t1);

StructuredBuffer<PointLight> lights : register(t2);

sampler clampSampler : register(s0);

cbuffer CameraPos : register(b0)
{
	float3 cameraPos;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 ambient = float3(0.1f, 0.1f, 0.1f);
	float shininessFactor = 5.0f;
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 specular = float3(0.0f, 0.0f, 0.0f);

	unsigned int nrOfLights = 0;
	unsigned int lightStride = 0;
	lights.GetDimensions(nrOfLights, lightStride);

	float3 diffuseMaterial = diffuseTexture.Sample(clampSampler, input.uv).xyz;
	float3 specularMaterial = specularTexture.Sample(clampSampler, input.uv).xyz;

	float3 position = input.worldPos;
	float3 normal = normalize(input.normal);
	float3 viewVector = normalize(cameraPos - position);

	for (unsigned int i = 0; i < nrOfLights; ++i)
	{
		float3 lightVector = normalize(lights[i].position - position);
		float cosA = max(0.0f, dot(lightVector, normal));
		diffuse += lights[i].colour * cosA;

		float3 reflection = reflect(-lightVector, normal);
		specular += lights[i].colour * max(0.0f, pow(dot(reflection, viewVector), shininessFactor));
	}

	float3 illumination = (ambient + diffuse) * diffuseMaterial +
		specularMaterial * specular;

	return float4(illumination, 1.0f);
}