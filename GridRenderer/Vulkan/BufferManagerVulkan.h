#pragma once

#include <optional>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "../BufferManager.h"

// TODO: Move info config.h or something to deduplicate
static constexpr uint32_t BACKBUFFER_COUNT = 2;

enum class BackingBufferType
{
    WRITE_ONCE,
    DYNAMIC
};

struct Buffer
{
    uint32_t elementCount;
    uint32_t sizeWithoutPadding;
    uint32_t sizeWithPadding;
    uint32_t backingBufferOffset;
    BackingBufferType backingBufferType;
};

constexpr uint32_t BACKING_BUFFER_SIZE = 1024 * 1024 * 16;
constexpr uint32_t BACKING_BUFFER_ALIGNMENT = 64; // TODO: Look up at runtime

struct BackingBuffer
{
    vk::UniqueDeviceMemory memory;
    vk::UniqueBuffer buffer;
    uint32_t size;
};

struct RoundRobinBuffer
{
    vk::UniqueDeviceMemory memory;
    vk::UniqueBuffer buffer;
    uint32_t totalSize;
    uint32_t chunkSize;
};

class BufferManagerVulkan: public BufferManager
{
  private:
    const vk::UniqueDevice& device;
    const vk::PhysicalDevice& physicalDevice;

    BackingBuffer writeOnceBackingBuffer;
    BackingBuffer dynamicBackingBuffer;
    RoundRobinBuffer roundRobinBuffer;
    std::vector<Buffer> buffers;

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

    vk::Buffer GetWriteOnceBackingBuffer();
    vk::Buffer GetDynamicBackingBuffer();
    vk::Buffer GetRoundRobinBuffer();
    uint32_t GetRoundRobinChunkSize();

    const Buffer& GetBuffer(ResourceIndex index);
    vk::Buffer GetBackingBuffer(ResourceIndex index);
};