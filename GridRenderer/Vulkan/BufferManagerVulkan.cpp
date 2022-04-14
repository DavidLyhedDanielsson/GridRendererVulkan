#include "BufferManagerVulkan.h"

BufferManagerVulkan::BufferManagerVulkan(const vk::UniqueDevice& device): device(device) {}

ResourceIndex BufferManagerVulkan::AddBuffer(
    void* data,
    unsigned int elementSize,
    unsigned int nrOfElements,
    PerFrameWritePattern cpuWrite,
    PerFrameWritePattern gpuWrite,
    unsigned int bindingFlags)
{
    return ResourceIndex(-1);
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