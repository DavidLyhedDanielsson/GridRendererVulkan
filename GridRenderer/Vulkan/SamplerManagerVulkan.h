#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../SamplerManager.h"

class SamplerManagerVulkan: public SamplerManager
{
  private:
    vk::UniqueDevice& device;

  public:
    SamplerManagerVulkan(vk::UniqueDevice&);
    SamplerManagerVulkan(const SamplerManagerVulkan& other) = delete;
    SamplerManagerVulkan& operator=(const SamplerManagerVulkan& other) = delete;
    SamplerManagerVulkan(SamplerManagerVulkan&& other) = default;
    SamplerManagerVulkan& operator=(SamplerManagerVulkan&& other) = default;

    ResourceIndex CreateSampler(SamplerType type, AddressMode adressMode) override;
};