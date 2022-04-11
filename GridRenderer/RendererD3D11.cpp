#include "RendererD3D11.h"

#include <stdexcept>
#include <SDL2/SDL_syswm.h>

void RendererD3D11::CreateBasicInterfaces(SDL_Window* window)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

	SDL_SysWMinfo sysInfo;
	SDL_VERSION(&sysInfo.version);
	SDL_GetWindowWMInfo(window, &sysInfo);


	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = sysInfo.info.win.window;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1, D3D11_SDK_VERSION,
		&swapChainDesc, &swapChain, &device, nullptr, &immediateContext);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create basic interfaces");
}

void RendererD3D11::CreateRenderTargetView()
{
	ID3D11Texture2D* backBuffer = nullptr;
	if (FAILED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&backBuffer))))
	{
		throw std::runtime_error("Could not fetch backbuffer from swap chain");
	}

	HRESULT hr = device->CreateRenderTargetView(backBuffer, NULL, &backBufferRTV);
	if(FAILED(hr))
		throw std::runtime_error("Could not create rtv for backbuffer");

	D3D11_TEXTURE2D_DESC desc;
	backBuffer->GetDesc(&desc);
	backBufferWidth = desc.Width;
	backBufferHeight = desc.Height;
	backBuffer->Release();

}

void RendererD3D11::CreateDepthStencil()
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = backBufferWidth;
	desc.Height = backBufferHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	if (FAILED(device->CreateTexture2D(&desc, nullptr, &depthBuffer)))
		throw std::runtime_error("Failed to create depth stencil texture!");

	if (FAILED(device->CreateDepthStencilView(depthBuffer, 0, &depthBufferDSV)))
		throw std::runtime_error("Failed to create dsv!");
}

void RendererD3D11::CreateViewport()
{
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.Width = static_cast<float>(backBufferWidth);
	viewport.Height = static_cast<float>(backBufferHeight);
}

void RendererD3D11::BindStructuredBuffer(ResourceIndex bufferIndex,
	PipelineShaderStage stage, std::uint8_t slot)
{
	ID3D11ShaderResourceView* srv = bufferManager.GetSRV(bufferIndex);

	switch (stage)
	{
	case PipelineShaderStage::VS:
		immediateContext->VSSetShaderResources(slot, 1, &srv);
		break;
	case PipelineShaderStage::PS:
		immediateContext->PSSetShaderResources(slot, 1, &srv);
		break;
	default:
		throw std::runtime_error("Incorrect shader stage for binding structured buffer");
	}
}

void RendererD3D11::BindConstantBuffer(ResourceIndex bufferIndex,
	PipelineShaderStage stage, std::uint8_t slot)
{
	ID3D11Buffer* buffer = bufferManager.GetBufferInterface(bufferIndex);

	switch (stage)
	{
	case PipelineShaderStage::VS:
		immediateContext->VSSetConstantBuffers(slot, 1, &buffer);
		break;
	case PipelineShaderStage::PS:
		immediateContext->PSSetConstantBuffers(slot, 1, &buffer);
		break;
	default:
		throw std::runtime_error("Incorrect shader stage for binding constant buffer");
	}
}

void RendererD3D11::BindBuffer(ResourceIndex bufferIndex, const PipelineBinding& binding)
{
	switch (binding.bindingType)
	{
	case PipelineBindingType::SHADER_RESOURCE:
		BindStructuredBuffer(bufferIndex, binding.shaderStage, binding.slotToBindTo);
		break;
	case PipelineBindingType::CONSTANT_BUFFER:
		BindConstantBuffer(bufferIndex, binding.shaderStage, binding.slotToBindTo);
		break;
	default:
		throw std::runtime_error("Incorrect bind type for binding buffer");
	}
}

void RendererD3D11::BindTextureSRV(ResourceIndex textureIndex, 
	PipelineShaderStage stage, std::uint8_t slot)
{
	ID3D11ShaderResourceView* srv = textureManager.GetSRV(textureIndex);

	switch (stage)
	{
	case PipelineShaderStage::VS:
		immediateContext->VSSetShaderResources(slot, 1, &srv);
		break;
	case PipelineShaderStage::PS:
		immediateContext->PSSetShaderResources(slot, 1, &srv);
		break;
	default:
		throw std::runtime_error("Incorrect shader stage for binding texture srv");
	}
}

void RendererD3D11::BindTexture(ResourceIndex textureIndex, const PipelineBinding& binding)
{
	switch (binding.bindingType)
	{
	case PipelineBindingType::SHADER_RESOURCE:
		BindTextureSRV(textureIndex, binding.shaderStage, binding.slotToBindTo);
		break;
	default:
		throw std::runtime_error("Incorrect bind type for binding texture");
	}
}

void RendererD3D11::BindSampler(ResourceIndex index,
	const PipelineBinding& binding)
{
	std::uint8_t slot = binding.slotToBindTo;
	ID3D11SamplerState* sampler = samplerManager.GetSampler(index);
	D3D11_SAMPLER_DESC temp;
	sampler->GetDesc(&temp);
	switch (binding.shaderStage)
	{
	case PipelineShaderStage::VS:
		immediateContext->VSSetSamplers(slot, 1, &sampler);
		break;
	case PipelineShaderStage::PS:
		immediateContext->PSSetSamplers(slot, 1, &sampler);
		break;
	default:
		throw std::runtime_error("Incorrect shader stage for binding sampler");
	}
}

void RendererD3D11::HandleBinding(const RenderObject& object, const PipelineBinding& binding)
{
	switch (binding.dataType)
	{
	case PipelineDataType::TRANSFORM:
		BindBuffer(object.GetTransformBufferIndex(), binding);
		break;
	case PipelineDataType::VERTEX:
		BindBuffer(object.GetMesh().GetVertexBuffer(), binding);
		break;
	case PipelineDataType::INDEX:
		BindBuffer(object.GetMesh().GetIndexBuffer(), binding);
		break;
	case PipelineDataType::DIFFUSE:
		BindTexture(object.GetSurfaceProperty().GetDiffuseTexture(), binding);
		break;
	case PipelineDataType::SPECULAR:
		BindTexture(object.GetSurfaceProperty().GetSpecularTexture(), binding);
		break;
	case PipelineDataType::SAMPLER:
		BindSampler(object.GetSurfaceProperty().GetSampler(), binding);
		break;
	default:
		throw std::runtime_error("Unknown data type for object binding");
	}
}

void RendererD3D11::HandleBinding(const PipelineBinding& binding)
{
	switch (binding.dataType)
	{
	case PipelineDataType::VIEW_PROJECTION:
		BindBuffer(currentCamera->GetVP(bufferManager), binding);
		break;
	case PipelineDataType::CAMERA_POS:
		BindBuffer(currentCamera->GetPosition(bufferManager), binding);
		break;
	case PipelineDataType::LIGHT:
		BindBuffer(lightBufferIndex, binding);
		break;
	case PipelineDataType::SAMPLER:
		BindSampler(currentRenderPass->GetGlobalSampler(
			binding.shaderStage, binding.slotToBindTo), binding);
		break;
	default:
		throw std::runtime_error("Unknown data type for global binding");
	}
}

RendererD3D11::RendererD3D11(SDL_Window* windowHandle)
{
	CreateBasicInterfaces(windowHandle);
	CreateRenderTargetView();
	CreateDepthStencil();
	CreateViewport();
	bufferManager.Initialise(device, immediateContext);
	textureManager.Initialise(device);
	samplerManager.Initialise(device);
}

RendererD3D11::~RendererD3D11()
{
	device->Release();
	immediateContext->Release();
	swapChain->Release();
	backBufferRTV->Release();
	depthBuffer->Release();
	depthBufferDSV->Release();
}

GraphicsRenderPass* RendererD3D11::CreateGraphicsRenderPass(
	const GraphicsRenderPassInfo& intialisationInfo)
{
	return new GraphicsRenderPassD3D11(device, intialisationInfo);
}

void RendererD3D11::DestroyGraphicsRenderPass(GraphicsRenderPass* pass)
{
	delete pass;
}

Camera* RendererD3D11::CreateCamera(float minDepth, float maxDepth, float aspectRatio)
{
	return new CameraD3D11(bufferManager, minDepth, maxDepth, aspectRatio);
}

void RendererD3D11::DestroyCamera(Camera* camera)
{
	delete camera;
}

BufferManager* RendererD3D11::GetBufferManager()
{
	return &bufferManager;
}

TextureManager* RendererD3D11::GetTextureManager()
{
	return &textureManager;
}

SamplerManager* RendererD3D11::GetSamplerManager()
{
	return &samplerManager;
}

void RendererD3D11::SetRenderPass(GraphicsRenderPass* toSet)
{
	currentRenderPass = static_cast<GraphicsRenderPassD3D11*>(toSet);
}

void RendererD3D11::SetCamera(Camera* toSet)
{
	currentCamera = static_cast<CameraD3D11*>(toSet);
}

void RendererD3D11::SetLightBuffer(ResourceIndex lightBufferIndexToUse)
{
	lightBufferIndex = lightBufferIndexToUse;
}

void RendererD3D11::PreRender()
{
	float clearColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	immediateContext->ClearRenderTargetView(backBufferRTV, clearColour);
	immediateContext->ClearDepthStencilView(depthBufferDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
	immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, &backBufferRTV, depthBufferDSV);
}

void RendererD3D11::Render(const std::vector<RenderObject>& objectsToRender)
{
	currentRenderPass->SetShaders(immediateContext);
	const std::vector<PipelineBinding>& objectBindings = currentRenderPass->GetObjectBindings();
	const std::vector<PipelineBinding>& globalBindings = currentRenderPass->GetGlobalBindings();

	for (auto& binding : globalBindings)
		HandleBinding(binding);

	for (auto& object : objectsToRender)
	{
		for (auto& binding : objectBindings)
			HandleBinding(object, binding);

		const Mesh& mesh = object.GetMesh();
		unsigned int drawCount = 0;
		if (mesh.GetIndexBuffer() != ResourceIndex(-1))
			drawCount = bufferManager.GetElementCount(mesh.GetIndexBuffer());
		else
			drawCount = bufferManager.GetElementCount(mesh.GetVertexBuffer());

		immediateContext->Draw(drawCount, 0);
	}
}

void RendererD3D11::Present()
{
	swapChain->Present(0, 0);
}
