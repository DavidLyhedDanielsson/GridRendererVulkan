#include "GraphicsRenderPassD3D11.h"

#include <fstream>
#include <stdexcept>

#include <d3dcompiler.h>

ID3DBlob* GraphicsRenderPassD3D11::LoadCSO(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Could not open CSO file");

	file.seekg(0, std::ios_base::end);
	size_t size = static_cast<size_t>(file.tellg());
	file.seekg(0, std::ios_base::beg);

	ID3DBlob* toReturn = nullptr;
	HRESULT hr = D3DCreateBlob(size, &toReturn);

	if (FAILED(hr))
		throw std::runtime_error("Could not create blob when loading CSO");

	file.read(static_cast<char*>(toReturn->GetBufferPointer()), size);
	file.close();

	return toReturn;
}

GraphicsRenderPassD3D11::GraphicsRenderPassD3D11(
	ID3D11Device* device, const GraphicsRenderPassInfo& info) : device(device),
	objectBindings(info.objectBindings), globalBindings(info.globalBindings)
{
	ID3DBlob* vsBlob = LoadCSO(info.vsPath);
	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		nullptr, &vertexShader);
	ID3DBlob* psBlob = LoadCSO(info.psPath);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
		nullptr, &pixelShader);
}

GraphicsRenderPassD3D11::~GraphicsRenderPassD3D11()
{
	vertexShader->Release();
	pixelShader->Release();
}

void GraphicsRenderPassD3D11::SetShaders(ID3D11DeviceContext* deviceContext)
{
	deviceContext->VSSetShader(vertexShader, nullptr, 0);
	deviceContext->PSSetShader(pixelShader, nullptr, 0);
}

const std::vector<PipelineBinding>& GraphicsRenderPassD3D11::GetObjectBindings()
{
	return objectBindings;
}

const std::vector<PipelineBinding>& GraphicsRenderPassD3D11::GetGlobalBindings()
{
	return globalBindings;
}

void GraphicsRenderPassD3D11::SetGlobalSampler(PipelineShaderStage shader,
	std::uint8_t slot, ResourceIndex index)
{
	switch (shader)
	{
	case PipelineShaderStage::VS:
		vsGlobalSamplers[slot] = index;
		break;
	case PipelineShaderStage::PS:
		psGlobalSamplers[slot] = index;
		break;
	default:
		throw std::runtime_error("Incorrect shader stage when setting sampler");
	}
}

ResourceIndex GraphicsRenderPassD3D11::GetGlobalSampler(PipelineShaderStage shader,
	std::uint8_t slot) const
{
	switch (shader)
	{
	case PipelineShaderStage::VS:
		return vsGlobalSamplers[slot];
		break;
	case PipelineShaderStage::PS:
		return psGlobalSamplers[slot];
		break;
	default:
		throw std::runtime_error("Incorrect shader stage when fetching sampler");
	}
}
