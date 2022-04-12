#pragma once

#include "../Renderer.h"

#include <SDL2/SDL_video.h>

class RendererVulkan: public Renderer
{
  public:
    RendererVulkan(SDL_Window* windowHandle);
    ~RendererVulkan() = default;
    RendererVulkan(const RendererVulkan& other) = delete;
    RendererVulkan& operator=(const RendererVulkan& other) = delete;
    RendererVulkan(RendererVulkan&& other) = default;
    RendererVulkan& operator=(RendererVulkan&& other) = default;

    GraphicsRenderPass* CreateGraphicsRenderPass(
        const GraphicsRenderPassInfo& intialisationInfo) override;
    void DestroyGraphicsRenderPass(GraphicsRenderPass* pass) override;

    Camera* CreateCamera(float minDepth, float maxDepth, float aspectRatio) override;
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