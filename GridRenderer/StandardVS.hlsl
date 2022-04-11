struct Vertex
{
	float3 position;
	float2 uv;
	float3 normal;
};

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float4 worldPos : WORLD_POSITION;
	float2 uv : UV;
	float3 normal : NORMAL;
};

cbuffer TransformBuffer : register(b0)
{
	float4x4 worldMatrix;
}

cbuffer CameraBuffer : register(b1)
{
	float4x4 vpMatrix;
}

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<unsigned int> indices : register(t1);

VertexShaderOutput main(uint vertexID : SV_VertexID)
{
	Vertex input = vertices[indices[vertexID]];
	VertexShaderOutput output;
	output.worldPos = mul(float4(input.position, 1.0f), worldMatrix);
	output.position = mul(output.worldPos, vpMatrix);
	output.uv = input.uv;
	output.normal = mul(float4(input.normal, 0.0f), worldMatrix);
	return output;
}