#include "BufferManagerVulkan.h"

#include <optional>

BufferManagerVulkan::BufferManagerVulkan(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
    : device(device)
    , physicalDevice(physicalDevice)
{
    auto buffer = device->createBufferUnique({
        .size = BACKING_BUFFER_SIZE,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer
                 | vk::BufferUsageFlagBits::eUniformBuffer, // TODO: Investigate, what is the
                                                            // performance implication of mixing?
        .sharingMode = vk::SharingMode::eExclusive,
        // Not used since eExclusive is used
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });

    vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(*buffer);
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    std::optional<uint32_t> memoryIndexOpt;
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool memoryTypeSupported = memoryRequirements.memoryTypeBits & (1 << i);

        if(memoryTypeSupported
           && memoryProperties.memoryTypes[i].propertyFlags
                  & vk::MemoryPropertyFlagBits::eHostCoherent) // Coherent for map functionality
        {
            // TODO: Investigate, are there reasons not to pick the first
            // compatible memory?
            memoryIndexOpt = i;
            break;
        }
    }
    assert(memoryIndexOpt.has_value());
    uint32_t memoryIndex = memoryIndexOpt.value();

    auto memory = device->allocateMemoryUnique({
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryIndex,
    });

    device->bindBufferMemory(*buffer, *memory, 0);

    this->backingBuffer.buffer = std::move(buffer);
    this->backingBuffer.memory = std::move(memory);
    this->backingBuffer.size = BACKING_BUFFER_SIZE;
}

ResourceIndex BufferManagerVulkan::AddBuffer(
    void* data,
    unsigned int elementSize,
    unsigned int nrOfElements,
    PerFrameWritePattern cpuWrite,
    PerFrameWritePattern gpuWrite,
    unsigned int bindingFlags)
{
    {
        const uint32_t bufferSizeWithoutPadding = elementSize * nrOfElements;
        const uint32_t bufferSizeWithPadding =
            bufferSizeWithoutPadding + BACKING_BUFFER_ALIGNMENT
            - (bufferSizeWithoutPadding % BACKING_BUFFER_ALIGNMENT);

        uint32_t bufferOffset =
            buffers.empty() ? 0
                            : buffers.back().backingBufferOffset + buffers.back().sizeWithPadding;

        buffers.push_back({
            .elementCount = nrOfElements,
            .sizeWithoutPadding = bufferSizeWithoutPadding,
            .sizeWithPadding = bufferSizeWithPadding,
            .backingBufferOffset = bufferOffset,
        });
    }
    const Buffer& buffer = buffers.back();

    // TODO: Use staging buffer instead of map for immediate performance gains
    // Don't forget to change the memory type from eHostCoherent!
    void* dataPtr = device->mapMemory(
        *backingBuffer.memory,
        buffer.backingBufferOffset,
        buffer.sizeWithoutPadding);
    std::memcpy(dataPtr, data, buffer.sizeWithoutPadding);
    device->unmapMemory(*backingBuffer.memory);

    return buffers.size() - 1;
}

void BufferManagerVulkan::UpdateBuffer(ResourceIndex index, void* data) {}

unsigned int BufferManagerVulkan::GetElementSize(ResourceIndex index)
{
    return -1;
}

unsigned int BufferManagerVulkan::GetElementCount(ResourceIndex index)
{
    return buffers[index].elementCount;
}

vk::Buffer BufferManagerVulkan::GetBackingBuffer()
{
    return *backingBuffer.buffer;
}

const Buffer& BufferManagerVulkan::GetBuffer(ResourceIndex index)
{
    return buffers[index];
}