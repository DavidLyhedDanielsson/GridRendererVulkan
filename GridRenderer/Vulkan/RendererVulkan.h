#pragma once

#include "../Renderer.h"

#include <SDL2/SDL_video.h>
// vulkan.hpp: https://github.com/KhronosGroup/Vulkan-Hpp
// Layer over vulkan to turn its C api into a more C++-like api.
// Vulkan functions have the same name, except that the vkXXX is now vk:XXX and some functions now
// belong to an instance e.g. vkEnumeratePhysicalDevices -> vk::Instance->enumeratePhysicalDevices

// vulkan_raii.hpp also includes smart (unique) pointers for vulkan types
#include <vulkan/vulkan_raii.hpp>

class RendererVulkan: public Renderer
{
  private:
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugCallback;
    vk::UniqueSurfaceKHR surface;
    uint32_t graphicsQueueIndex;
    vk::UniqueDevice device;

    vk::UniqueRenderPass renderPass;

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