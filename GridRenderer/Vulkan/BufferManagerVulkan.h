#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../BufferManager.h"

class BufferManagerVulkan: public BufferManager
{
  private:
    const vk::UniqueDevice& device;
    const vk::PhysicalDevice& physicalDevice;

    std::vector<vk::UniqueDeviceMemory> memories;
    std::vector<vk::UniqueBuffer> buffers;

  public:
    BufferManagerVulkan(const vk::UniqueDevice& device, const vk::PhysicalDevice& physicalDevice);
    BufferManagerVulkan(const BufferManagerVulkan& other) = delete;
    BufferManagerVulkan& operator=(const BufferManagerVulkan& other) = delete;
    BufferManagerVulkan(BufferManagerVulkan&& other) = default;
    BufferManagerVulkan& operator=(BufferManagerVulkan&& other) = default;

    ResourceIndex AddBuffer(
        void* data,
        unsigned int elementSize,
        unsigned int nrOfElements,
        PerFrameWritePattern cpuWrite,
        PerFrameWritePattern gpuWrite,
        unsigned int bindingFlags) override;

    void UpdateBuffer(ResourceIndex index, void* data) override;
    unsigned int GetElementSize(ResourceIndex index) override;
    unsigned int GetElementCount(ResourceIndex index) override;
};