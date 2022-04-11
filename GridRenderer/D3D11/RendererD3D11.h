#pragma once

#include <d3d11_4.h>
#include <dxgi1_6.h>

#include "../Renderer.h"
#include "GraphicsRenderPassD3D11.h"
#include "BufferManagerD3D11.h"
#include "TextureManagerD3D11.h"
#include "SamplerManagerD3D11.h"
#include "CameraD3D11.h"

#include <SDL2/SDL_video.h>

class RendererD3D11 : public Renderer
{
private:
	unsigned int backBufferWidth = 0;
	unsigned int backBufferHeight = 0;

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* immediateContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11RenderTargetView* backBufferRTV = nullptr;
	ID3D11Texture2D* depthBuffer = nullptr;
	ID3D11DepthStencilView* depthBufferDSV = nullptr;
	D3D11_VIEWPORT viewport;

	BufferManagerD3D11 bufferManager;
	TextureManagerD3D11 textureManager;
	SamplerManagerD3D11 samplerManager;

	GraphicsRenderPassD3D11* currentRenderPass = nullptr;
	CameraD3D11* currentCamera = nullptr;
	ResourceIndex lightBufferIndex = ResourceIndex(-1);

	void CreateBasicInterfaces(SDL_Window* windowHandle);
	void CreateRenderTargetView();
	void CreateDepthStencil();
	void CreateViewport();

	void BindStructuredBuffer(ResourceIndex bufferIndex,
		PipelineShaderStage stage, std::uint8_t slot);
	void BindConstantBuffer(ResourceIndex bufferIndex,
		PipelineShaderStage stage, std::uint8_t slot);
	void BindBuffer(ResourceIndex bufferIndex, const PipelineBinding& binding);

	void BindTextureSRV(ResourceIndex textureIndex,
		PipelineShaderStage stage, std::uint8_t slot);
	void BindTexture(ResourceIndex textureIndex, const PipelineBinding& binding);

	void BindSampler(ResourceIndex index, const PipelineBinding& binding);

	void HandleBinding(const RenderObject& object,
		const PipelineBinding& binding);

	void HandleBinding(const PipelineBinding& binding);

public:
	RendererD3D11(SDL_Window* windowHandle);
	virtual ~RendererD3D11();
	RendererD3D11(const RendererD3D11& other) = delete;
	RendererD3D11& operator=(const RendererD3D11& other) = delete;
	RendererD3D11(RendererD3D11&& other) = default;
	RendererD3D11& operator=(RendererD3D11&& other) = default;

	GraphicsRenderPass* CreateGraphicsRenderPass(
		const GraphicsRenderPassInfo& intialisationInfo) override;
	void DestroyGraphicsRenderPass(GraphicsRenderPass* pass) override;

	Camera* CreateCamera(float minDepth, float maxDepth,
		float aspectRatio) override;
	void DestroyCamera(Camera* camera) override;

	BufferManager* GetBufferManager() override;
	TextureManager* GetTextureManager() override;
	SamplerManager* GetSamplerManager() override;

	void SetRenderPass(GraphicsRenderPass* toSet) override;
	void SetCamera(Camera* toSet) override;
	void SetLightBuffer(ResourceIndex lightBufferIndexToUse) override;

	void PreRender() override;
	void Render(const std::vector<RenderObject>& objectsToRender) override;
	void Present() override;

};