#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../SamplerManager.h"

struct SamplerData
{
    vk::UniqueSampler sampler;
    vk::UniqueDescriptorSet descriptorSet;
};

class SamplerManagerVulkan: public SamplerManager
{
  private:
    const vk::UniqueDevice& device;
    const vk::UniqueDescriptorSetLayout& samplerSetLayout;

    vk::UniqueDescriptorPool descriptorPool;
    std::vector<SamplerData> samplers;

  public:
    SamplerManagerVulkan(const vk::UniqueDevice&, const vk::UniqueDescriptorSetLayout&);
    SamplerManagerVulkan(const SamplerManagerVulkan& other) = delete;
    SamplerManagerVulkan& operator=(const SamplerManagerVulkan& other) = delete;
    SamplerManagerVulkan(SamplerManagerVulkan&& other) = default;
    SamplerManagerVulkan& operator=(SamplerManagerVulkan&& other) = delete;

    ResourceIndex CreateSampler(SamplerType type, AddressMode adressMode) override;
    const vk::DescriptorSet& GetDescriptorSet(ResourceIndex index);
};