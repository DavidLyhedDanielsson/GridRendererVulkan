#pragma once

#include "../Renderer.h"

#include <SDL2/SDL_video.h>
// vulkan.hpp: https://github.com/KhronosGroup/Vulkan-Hpp
// Layer over vulkan to turn its C api into a more C++-like api.
// Vulkan functions have the same name, except that the vkXXX is now vk:XXX and some functions now
// belong to an instance e.g. vkEnumeratePhysicalDevices -> vk::Instance->enumeratePhysicalDevices

// vulkan_raii.hpp also includes smart (unique) pointers for vulkan types
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <memory>
#include <optional>

#include "BufferManagerVulkan.h"
#include "CameraVulkan.h"
#include "GraphicsRenderPassVulkan.h"
#include "SamplerManagerVulkan.h"
#include "TextureManagerVulkan.h"

class RendererVulkan: public Renderer
{
  private:
    // 2 for double buffering, 3 for triple, etc.
    static constexpr uint32_t BACKBUFFER_COUNT = 2;

    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugCallback;
    vk::UniqueSurfaceKHR surface;
    uint32_t graphicsQueueIndex;
    vk::Queue graphicsQueue;
    vk::UniqueDevice device;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueSwapchainKHR swapchain;
    std::array<vk::UniqueFence, BACKBUFFER_COUNT> queueDoneFences;
    std::array<vk::UniqueSemaphore, BACKBUFFER_COUNT> imageAvailableSemaphores;
    std::array<vk::UniqueSemaphore, BACKBUFFER_COUNT> renderFinishedSemaphores;

    vk::UniqueRenderPass renderPass;
    std::vector<vk::UniqueFramebuffer> framebuffers;
    std::vector<vk::UniqueImageView> backbufferImageViews;
    vk::UniqueDescriptorSetLayout vertexIndexDescriptorSetLayout;
    vk::UniqueDescriptorSetLayout transformBufferDescriptorSetLayout;
    vk::UniqueDescriptorSetLayout viewProjectionDescriptorSetLayout;
    vk::UniqueDescriptorSet vertexIndexDescriptorSet;
    vk::UniqueDescriptorSet transformDescriptorSet;
    vk::UniqueDescriptorSet viewProjectionDescriptorSet;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeline;

    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;

    std::optional<CameraVulkan> cameraOpt;
    ResourceIndex cameraBufferIndex;

    // Use dynamic memory so that the sampler manager can be initialized with a reference to
    // this->device
    std::unique_ptr<SamplerManagerVulkan> samplerManager;
    std::unique_ptr<BufferManagerVulkan> bufferManager;
    std::unique_ptr<TextureManagerVulkan> textureManager;

    std::vector<GraphicsRenderPassVulkan> renderPasses;
    std::vector<vk::UniqueShaderModule> shaderModules;

    vk::UniqueDescriptorPool descriptorPool;

    uint64_t currentFrame;
    // Can't do currentFrame % BACKBUFFER_COUNT since the spec does not define the order of
    // swapchain images
    uint32_t currentSwapchainImageIndex;

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