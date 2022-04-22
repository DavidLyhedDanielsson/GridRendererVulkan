#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../TextureManager.h"

struct TextureData
{
    vk::UniqueImage iamge;
    vk::UniqueImageView imageView;
    vk::UniqueDeviceMemory imageMemory;
};

class TextureManagerVulkan: public TextureManager
{
  private:
    const vk::UniqueDevice& device;
    const vk::PhysicalDevice& physicalDevice;
    const vk::Queue& uploadQueue;
    const uint32_t queueFamilyIndex;

    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;

    vk::UniqueCommandPool commandPool;
    vk::UniqueCommandBuffer commandBuffer;

    std::vector<TextureData> textures;

  public:
    TextureManagerVulkan(
        const vk::UniqueDevice& device,
        const vk::PhysicalDevice& physicalDevice,
        const vk::Queue& uploadQueue,
        uint32_t queueFamilyIndex);
    TextureManagerVulkan(const TextureManagerVulkan& other) = delete;
    TextureManagerVulkan& operator=(const TextureManagerVulkan& other) = delete;
    TextureManagerVulkan(TextureManagerVulkan&& other) = default;
    TextureManagerVulkan& operator=(TextureManagerVulkan&& other) = default;

    ResourceIndex AddTexture(void* textureData, const TextureInfo& textureInfo) override;
};