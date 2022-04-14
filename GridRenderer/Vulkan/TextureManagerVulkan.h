#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../TextureManager.h"

class TextureManagerVulkan: public TextureManager
{
  private:
  public:
    TextureManagerVulkan() = default;
    TextureManagerVulkan(const TextureManagerVulkan& other) = delete;
    TextureManagerVulkan& operator=(const TextureManagerVulkan& other) = delete;
    TextureManagerVulkan(TextureManagerVulkan&& other) = default;
    TextureManagerVulkan& operator=(TextureManagerVulkan&& other) = default;

    ResourceIndex AddTexture(void* textureData, const TextureInfo& textureInfo) override;
};