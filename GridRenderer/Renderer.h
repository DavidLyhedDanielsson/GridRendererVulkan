#pragma once

#include "BufferManager.h"
#include "TextureManager.h"
#include "SamplerManager.h"
#include "GraphicsRenderPass.h"
#include "Camera.h"

class Renderer
{
protected:

public:
	Renderer() = default;
	virtual ~Renderer() = default;
	Renderer(const Renderer& other) = delete;
	Renderer& operator=(const Renderer& other) = delete;
	Renderer(Renderer&& other) = default;
	Renderer& operator=(Renderer&& other) = default;

	virtual GraphicsRenderPass* CreateGraphicsRenderPass(
		const GraphicsRenderPassInfo& intialisationInfo) = 0;
	virtual void DestroyGraphicsRenderPass(GraphicsRenderPass* pass) = 0;

	virtual Camera* CreateCamera(float minDepth, float maxDepth,
		float aspectRatio) = 0;
	virtual void DestroyCamera(Camera* camera) = 0;

	virtual BufferManager* GetBufferManager() = 0;
	virtual TextureManager* GetTextureManager() = 0;
	virtual SamplerManager* GetSamplerManager() = 0;

	virtual void SetRenderPass(GraphicsRenderPass* toSet) = 0;
	virtual void SetCamera(Camera* toSet) = 0;
	virtual void SetLightBuffer(ResourceIndex lightBufferIndexToUse) = 0;

	virtual void PreRender() = 0;
	virtual void Render(const std::vector<RenderObject>& objectsToRender) = 0;
	virtual void Present() = 0;
};