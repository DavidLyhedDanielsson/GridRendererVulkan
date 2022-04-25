#include "SamplerManagerVulkan.h"

SamplerManagerVulkan::SamplerManagerVulkan(
    const vk::UniqueDevice& device,
    const vk::UniqueDescriptorSetLayout& samplerSetLayout)
    : device(device)
    , samplerSetLayout(samplerSetLayout)
{
    vk::DescriptorPoolSize samplerInfo = {
        .type = vk::DescriptorType::eSampler,
        .descriptorCount = 1,
    };
    vk::DescriptorPoolCreateInfo poolInfo = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 10,
        .poolSizeCount = 1,
        .pPoolSizes = &samplerInfo,
    };
    this->descriptorPool = device->createDescriptorPoolUnique(poolInfo);
}

ResourceIndex SamplerManagerVulkan::CreateSampler(SamplerType type, AddressMode adressMode)
{
    vk::SamplerCreateInfo samplerInfo = {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0,
        .anisotropyEnable = false, // TODO
        .maxAnisotropy = 1,
        .compareEnable = false,
        .compareOp = vk::CompareOp::eNever,
        .minLod = 0,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        .unnormalizedCoordinates = false,
    };
    auto sampler = device->createSamplerUnique(samplerInfo);

    vk::DescriptorSetAllocateInfo descSetInfo = {
        .descriptorPool = *this->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*this->samplerSetLayout,
    };
    auto descriptorSet = std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);

    vk::DescriptorImageInfo descriptorImageInfo = {
        .sampler = *sampler,
        .imageView = VK_NULL_HANDLE,
    };
    vk::WriteDescriptorSet imageWriteDescriptor = {
        .dstSet = *descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eSampler,
        .pImageInfo = &descriptorImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };
    device->updateDescriptorSets(1, &imageWriteDescriptor, 0, nullptr);

    samplers.push_back({.sampler = std::move(sampler), .descriptorSet = std::move(descriptorSet)});
    return samplers.size() - 1;
}

const vk::DescriptorSet& SamplerManagerVulkan::GetDescriptorSet(ResourceIndex index)
{
    return *samplers[index].descriptorSet;
}