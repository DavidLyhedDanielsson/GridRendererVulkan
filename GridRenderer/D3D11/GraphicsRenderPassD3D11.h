#pragma once

#include <d3d11_4.h>
#include <array>

#include "../GraphicsRenderPass.h"

class GraphicsRenderPassD3D11 : public GraphicsRenderPass
{
private:
	ID3D11Device* device = nullptr;
	std::vector<PipelineBinding> objectBindings;
	std::vector<PipelineBinding> globalBindings;
	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;

	std::array<ResourceIndex, 
		D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> vsGlobalSamplers;
	std::array<ResourceIndex, 
		D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> psGlobalSamplers;

	ID3DBlob* LoadCSO(const std::string& filepath);

public:
	GraphicsRenderPassD3D11(ID3D11Device* device, const GraphicsRenderPassInfo& info);
	virtual ~GraphicsRenderPassD3D11();
	GraphicsRenderPassD3D11(const GraphicsRenderPassD3D11& other) = delete;
	GraphicsRenderPassD3D11& operator=(const GraphicsRenderPassD3D11& other) = delete;
	GraphicsRenderPassD3D11(GraphicsRenderPassD3D11&& other) = default;
	GraphicsRenderPassD3D11& operator=(GraphicsRenderPassD3D11&& other) = default;

	void SetShaders(ID3D11DeviceContext* deviceContext);
	const std::vector<PipelineBinding>& GetObjectBindings();
	const std::vector<PipelineBinding>& GetGlobalBindings();

	void SetGlobalSampler(PipelineShaderStage shader, 
		std::uint8_t slot, ResourceIndex index) override;
	ResourceIndex GetGlobalSampler(PipelineShaderStage shader,
		std::uint8_t slot) const;
};