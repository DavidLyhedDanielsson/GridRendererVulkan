#include "BufferManagerVulkan.h"

#include <optional>

BufferManagerVulkan::BufferManagerVulkan(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
    : device(device)
    , physicalDevice(physicalDevice)
{
}

ResourceIndex BufferManagerVulkan::AddBuffer(
    void* data,
    unsigned int elementSize,
    unsigned int nrOfElements,
    PerFrameWritePattern cpuWrite,
    PerFrameWritePattern gpuWrite,
    unsigned int bindingFlags)
{
    auto buffer = device->createBufferUnique({
        .size = elementSize * nrOfElements,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
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

    // TODO: Allocate big buffer for multiple buffers instead of one allocation
    // per buffer
    auto memory = device->allocateMemoryUnique({
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryIndex,
    });

    device->bindBufferMemory(*buffer, *memory, 0);

    memories.push_back(std::move(memory));
    buffers.push_back(std::move(buffer));

    return ResourceIndex(buffers.size() - 1);
}

void BufferManagerVulkan::UpdateBuffer(ResourceIndex index, void* data) {}

unsigned int BufferManagerVulkan::GetElementSize(ResourceIndex index)
{
    return -1;
}

unsigned int BufferManagerVulkan::GetElementCount(ResourceIndex index)
{
    return -1;
}