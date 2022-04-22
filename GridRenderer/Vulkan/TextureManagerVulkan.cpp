#include "TextureManagerVulkan.h"

#include <optional>

#include "StlHelpers/EntireCollection.h"

TextureManagerVulkan::TextureManagerVulkan(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::Queue& uploadQueue,
    uint32_t queueFamilyIndex)
    : device(device)
    , physicalDevice(physicalDevice)
    , uploadQueue(uploadQueue)
    , queueFamilyIndex(queueFamilyIndex)
{
    vk::CommandPoolCreateInfo commandPoolInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndex,
    };
    this->commandPool = device->createCommandPoolUnique(commandPoolInfo);
    vk::CommandBufferAllocateInfo commandBufferInfo = {
        .commandPool = *this->commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    this->commandBuffer = std::move(device->allocateCommandBuffersUnique(commandBufferInfo)[0]);

    vk::BufferCreateInfo bufferInfo = {
        .size = 4096 * 4096 * 4 * sizeof(float),
        .usage = vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &this->queueFamilyIndex,
    };
    this->stagingBuffer = device->createBufferUnique(bufferInfo);

    vk::MemoryRequirements memoryRequirements =
        device->getBufferMemoryRequirements(*this->stagingBuffer);
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    std::optional<uint32_t> memoryIndexOpt;
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool memoryTypeSupported = memoryRequirements.memoryTypeBits & (1 << i);

        if(memoryTypeSupported
           && memoryProperties.memoryTypes[i].propertyFlags
                  & (vk::MemoryPropertyFlagBits::eHostCoherent
                     | vk::MemoryPropertyFlagBits::eHostVisible))
        {
            memoryIndexOpt = i;
            break;
        }
    }
    assert(memoryIndexOpt.has_value());
    uint32_t memoryIndex = memoryIndexOpt.value();

    vk::MemoryAllocateInfo memoryInfo = {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryIndex,
    };
    this->stagingBufferMemory = device->allocateMemoryUnique(memoryInfo);

    device->bindBufferMemory(*stagingBuffer, *stagingBufferMemory, 0);
}

std::optional<vk::Format> convertVkFormat(const FormatInfo& info)
{
    using namespace std;
    using TCC = TexelComponentCount;
    using TCS = TexelComponentSize;
    using TCT = TexelComponentType;
    vector<pair<tuple<TexelComponentCount, TexelComponentSize, TexelComponentType>, vk::Format>>
        matcher = {
            // Single byte float not available
            make_pair(make_tuple(TCC::SINGLE, TCS::BYTE, TCT::UNORM), vk::Format::eR8Unorm),
            // Single byte depth not available
            make_pair(make_tuple(TCC::SINGLE, TCS::WORD, TCT::FLOAT), vk::Format::eR32Sfloat),
            // Single word unorm not available
            // Single word depth not available

            // Quad byte float not available
            make_pair(make_tuple(TCC::QUAD, TCS::BYTE, TCT::UNORM), vk::Format::eR8G8B8A8Unorm),
            // Quad byte depth not available
            // Quad word float not available
            make_pair(
                make_tuple(TCC::QUAD, TCS::WORD, TCT::FLOAT),
                vk::Format::eR32G32B32A32Sfloat),
            // Quad word unorm not available
            // Quad word depth not available
        };
    auto iter = std::find_if(entire_collection(matcher), [&](const auto& pair) {
        return pair.first
               == make_tuple(info.componentCount, info.componentSize, info.componentType);
    });

    if(iter != matcher.end())
        return iter->second;
    else
        return std::nullopt;
}

ResourceIndex TextureManagerVulkan::AddTexture(void* textureData, const TextureInfo& textureInfo)
{
    auto vkFormatOpt = convertVkFormat(textureInfo.format);
    assert(vkFormatOpt);
    vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = *vkFormatOpt,
        .extent =
            {
                .width = textureInfo.baseTextureWidth,
                .height = textureInfo.baseTextureHeight,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &this->queueFamilyIndex,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    vk::UniqueImage image = device->createImageUnique(imageInfo);

    uint32_t componentCount = textureInfo.format.componentCount == TexelComponentCount::SINGLE ? 1
                              : textureInfo.format.componentCount == TexelComponentCount::QUAD ? 4
                                                                                               : -1;
    uint32_t componentSize = textureInfo.format.componentSize == TexelComponentSize::BYTE   ? 1
                             : textureInfo.format.componentSize == TexelComponentSize::WORD ? 4
                                                                                            : -1;

    // Very simple (and slow) synchronization
    device->waitIdle();
    void* memory = device->mapMemory(*stagingBufferMemory, 0, VK_WHOLE_SIZE);
    std::memcpy(
        memory,
        textureData,
        textureInfo.baseTextureWidth * textureInfo.baseTextureHeight * componentCount
            * componentSize);
    device->unmapMemory(*stagingBufferMemory);

    vk::MemoryRequirements memoryRequirements = device->getImageMemoryRequirements(*image);
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    std::optional<uint32_t> memoryIndexOpt;
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool memoryTypeSupported = memoryRequirements.memoryTypeBits & (1 << i);

        if(memoryTypeSupported
           && memoryProperties.memoryTypes[i].propertyFlags
                  & vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            memoryIndexOpt = i;
            break;
        }
    }
    assert(memoryIndexOpt.has_value());
    uint32_t memoryIndex = memoryIndexOpt.value();

    vk::MemoryAllocateInfo memoryInfo = {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryIndex,
    };
    auto imageMemory = device->allocateMemoryUnique(memoryInfo);
    device->bindImageMemory(*image, *imageMemory, 0);

    commandBuffer->begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });

    vk::ImageMemoryBarrier memoryBarrier = {
        .srcAccessMask = vk::AccessFlags(),
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = *image,
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    commandBuffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits::eByRegion,
        {},
        {},
        {memoryBarrier});

    vk::BufferImageCopy copyData = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        .imageOffset =
            {
                .x = 0,
                .y = 0,
                .z = 0,
            },
        .imageExtent =
            {
                .width = textureInfo.baseTextureWidth,
                .height = textureInfo.baseTextureHeight,
                .depth = 1,
            },
    };
    commandBuffer->copyBufferToImage(
        *stagingBuffer,
        *image,
        vk::ImageLayout::eTransferDstOptimal,
        {copyData});

    memoryBarrier = {
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = *image,
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    commandBuffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::eByRegion,
        {},
        {},
        {memoryBarrier});

    commandBuffer->end();
    vk::SubmitInfo submitInfo = {
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    this->uploadQueue.submit(submitInfo);
    device->waitIdle();
    return 0;
}
