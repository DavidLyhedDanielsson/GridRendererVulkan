#include "BufferManagerVulkan.h"

#include <optional>

#include "StlHelpers/EntireCollection.h"

BackingBuffer createWriteOnceBackingBuffer(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
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

    return BackingBuffer{
        .memory = std::move(memory),
        .buffer = std::move(buffer),
        .size = BACKING_BUFFER_SIZE,
    };
}

BackingBuffer createDynamicBackingBuffer(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
{
    auto buffer = device->createBufferUnique({
        .size = BACKING_BUFFER_SIZE,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eUniformBuffer
                 | vk::BufferUsageFlagBits::eTransferSrc, // TODO: Investigate, what is the
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

    return BackingBuffer{
        .memory = std::move(memory),
        .buffer = std::move(buffer),
        .size = BACKING_BUFFER_SIZE,
    };
}

RoundRobinBuffer createRoundRobinBuffer(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
{
    auto buffer = device->createBufferUnique({
        .size = BACKING_BUFFER_SIZE * BACKBUFFER_COUNT,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eUniformBuffer
                 | vk::BufferUsageFlagBits::eTransferDst,
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

    return RoundRobinBuffer{
        .memory = std::move(memory),
        .buffer = std::move(buffer),
        .totalSize = BACKING_BUFFER_SIZE * BACKBUFFER_COUNT,
        .chunkSize = BACKING_BUFFER_SIZE,
    };
}

BufferManagerVulkan::BufferManagerVulkan(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
    : device(device)
    , physicalDevice(physicalDevice)
    , writeOnceBackingBuffer(createWriteOnceBackingBuffer(device, physicalDevice))
    , dynamicBackingBuffer(createDynamicBackingBuffer(device, physicalDevice))
    , roundRobinBuffer(createRoundRobinBuffer(device, physicalDevice))
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
    auto backingBufferType = cpuWrite == PerFrameWritePattern::NEVER ? BackingBufferType::WRITE_ONCE
                                                                     : BackingBufferType::DYNAMIC;

    {
        auto lastMatchingBuffer =
            std::find_if(buffers.rbegin(), buffers.rend(), [=](const Buffer& buffer) {
                return buffer.backingBufferType == backingBufferType;
            });

        uint32_t bufferOffset = 0;
        if(lastMatchingBuffer != buffers.rend())
        {
            bufferOffset =
                lastMatchingBuffer->backingBufferOffset + lastMatchingBuffer->sizeWithPadding;
        }

        const uint32_t bufferSizeWithoutPadding = elementSize * nrOfElements;
        const uint32_t bufferSizeWithPadding =
            (bufferSizeWithoutPadding + BACKING_BUFFER_ALIGNMENT - 1) / BACKING_BUFFER_ALIGNMENT
            * BACKING_BUFFER_ALIGNMENT;

        buffers.push_back({
            .elementCount = nrOfElements,
            .sizeWithoutPadding = bufferSizeWithoutPadding,
            .sizeWithPadding = bufferSizeWithPadding,
            .backingBufferOffset = bufferOffset,
            .backingBufferType = backingBufferType,
        });
    }
    const Buffer& buffer = buffers.back();

    // TODO: Use staging buffer instead of map for immediate performance gains
    // Don't forget to change the memory type from eHostCoherent!
    void* dataPtr = device->mapMemory(
        (backingBufferType == BackingBufferType::WRITE_ONCE ? *writeOnceBackingBuffer.memory
                                                            : *dynamicBackingBuffer.memory),
        buffer.backingBufferOffset,
        buffer.sizeWithoutPadding);
    std::memcpy(dataPtr, data, buffer.sizeWithoutPadding);
    device->unmapMemory(
        (backingBufferType == BackingBufferType::WRITE_ONCE ? *writeOnceBackingBuffer.memory
                                                            : *dynamicBackingBuffer.memory));

    return buffers.size() - 1;
}

void BufferManagerVulkan::UpdateBuffer(ResourceIndex index, void* data)
{
    const Buffer& buffer = buffers[index];
    assert(buffer.backingBufferType == BackingBufferType::WRITE_ONCE);
    const BackingBuffer& backingBuffer = writeOnceBackingBuffer;

    void* dataPtr = device->mapMemory(
        *backingBuffer.memory,
        buffer.backingBufferOffset,
        buffer.sizeWithoutPadding);
    std::memcpy(dataPtr, data, buffer.sizeWithoutPadding);
    device->unmapMemory(*backingBuffer.memory);
}

unsigned int BufferManagerVulkan::GetElementSize(ResourceIndex index)
{
    return -1;
}

unsigned int BufferManagerVulkan::GetElementCount(ResourceIndex index)
{
    return buffers[index].elementCount;
}

vk::Buffer BufferManagerVulkan::GetWriteOnceBackingBuffer()
{
    return *writeOnceBackingBuffer.buffer;
}

vk::Buffer BufferManagerVulkan::GetDynamicBackingBuffer()
{
    return *dynamicBackingBuffer.buffer;
}

vk::Buffer BufferManagerVulkan::GetRoundRobinBuffer()
{
    return *roundRobinBuffer.buffer;
}

uint32_t BufferManagerVulkan::GetRoundRobinChunkSize()
{
    return roundRobinBuffer.chunkSize;
}

const Buffer& BufferManagerVulkan::GetBuffer(ResourceIndex index)
{
    return buffers[index];
}

vk::Buffer BufferManagerVulkan::GetBackingBuffer(ResourceIndex index)
{
    return buffers[index].backingBufferType == BackingBufferType::WRITE_ONCE
               ? *writeOnceBackingBuffer.buffer
               : *dynamicBackingBuffer.buffer;
}
