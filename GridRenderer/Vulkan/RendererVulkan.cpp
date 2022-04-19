#include "RendererVulkan.h"

#include <array>
#include <iostream> // Only used for cerr in the debug callback
#include <map>
#include <memory>
#include <optional>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#include "FileUtils.h"
#include "StlHelpers/EntireCollection.h"

// Small macro to avoid typos when typing the function name
#define vkGetInstanceProcAddrQ(instance, func) (PFN_##func) instance->getProcAddr(#func)
// https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/CreateDebugUtilsMessenger/CreateDebugUtilsMessenger.cpp
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    VkAllocationCallbacks const* pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        return VK_FALSE;

    // vkCreateSwapchainKHR called with invalid imageExtent. This randomly happens when resizing
    if(pCallbackData->messageIdNumber == 0x7cd0911d)
        return VK_FALSE;

    std::cerr << pCallbackData->pMessage << std::endl;
    assert(false);
}

vk::UniqueInstance createInstance(std::vector<const char*> requiredExtensions)
{
    // Required for the validation layers
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Enable validation layer, otherwise no error checking will be done
    std::map<std::string, bool> requiredLayers = {{"VK_LAYER_KHRONOS_validation", false}};

    // Make sure all required layers are available
    std::vector<vk::LayerProperties> properties = vk::enumerateInstanceLayerProperties();
    std::for_each(entire_collection(requiredLayers), [&](const auto& pair) {
        if(std::find_if(
               entire_collection(properties),
               [&](const auto& property) { return pair.first == property.layerName; })
           == properties.end())
            throw std::runtime_error(std::string("Missing required vulkan layer ") + pair.first);
    });

    vk::ApplicationInfo applicationInfo = {
        .pApplicationName = "BthVulkan",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "BthVulkan",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    std::vector<const char*> enabledLayerNames;
    std::transform(
        entire_collection(requiredLayers),
        std::back_inserter(enabledLayerNames),
        [](const auto& pair) { return pair.first.c_str(); });

    vk::InstanceCreateInfo instanceCreateInfo = {
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = (uint32_t)enabledLayerNames.size(),
        .ppEnabledLayerNames = enabledLayerNames.data(),
        .enabledExtensionCount = (uint32_t)requiredExtensions.size(),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    // Use structured bindings if using exceptions
    // auto [res, instance] = vk::createInstanceUnique
    return vk::createInstanceUnique(instanceCreateInfo);
}

vk::UniqueDebugUtilsMessengerEXT initializeDebugCallback(const vk::UniqueInstance& instance)
{
    pfnVkCreateDebugUtilsMessengerEXT =
        vkGetInstanceProcAddrQ(*instance, vkCreateDebugUtilsMessengerEXT);
    assert(pfnVkCreateDebugUtilsMessengerEXT);

    pfnVkDestroyDebugUtilsMessengerEXT =
        vkGetInstanceProcAddrQ(*instance, vkDestroyDebugUtilsMessengerEXT);
    assert(pfnVkCreateDebugUtilsMessengerEXT);

    vk::DebugUtilsMessengerCreateInfoEXT msgCreateInfo = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                           | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                           | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                       | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                       | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };
    return instance->createDebugUtilsMessengerEXTUnique(msgCreateInfo);
}

vk::UniqueSurfaceKHR createSurface(SDL_Window* handle, const vk::UniqueInstance& instance)
{
    VkSurfaceKHR surfaceRaw;
    if(!SDL_Vulkan_CreateSurface(handle, *instance, &surfaceRaw))
        throw std::runtime_error("Cannot create vulkan surface");
    return vk::UniqueSurfaceKHR(surfaceRaw);
}

struct DeviceInfo
{
    vk::UniqueDevice device;
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueIndex;
};

DeviceInfo createDevice(const vk::UniqueInstance& instance, const vk::UniqueSurfaceKHR& surface)
{
    std::vector<vk::PhysicalDevice> pDevices = instance->enumeratePhysicalDevices();
    std::optional<vk::PhysicalDevice> pickedPDeviceOpt;
    uint32_t graphicsQueueIndex = 0; // Will be set if pickedPDeviceOpt is set
    std::for_each(entire_collection(pDevices), [&](const vk::PhysicalDevice& pDevice) {
        std::vector<vk::ExtensionProperties> extensions =
            pDevice.enumerateDeviceExtensionProperties();

        bool hasSwapchainSupport =
            std::find_if(
                entire_collection(extensions),
                [](const auto& extension) {
                    return strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
                })
            != extensions.end();
        if(!hasSwapchainSupport)
            return;

        std::vector<vk::QueueFamilyProperties> queueProperties = pDevice.getQueueFamilyProperties();
        std::optional<uint32_t> graphicsQueueIndexOpt;
        for(uint32_t i = 0; i < (uint32_t)queueProperties.size(); ++i)
        {
            const auto& properties = queueProperties[i];

            if(!(properties.queueFlags & vk::QueueFlagBits::eGraphics))
                continue;

            if(!pDevice.getSurfaceSupportKHR(i, *surface))
                continue;

            graphicsQueueIndexOpt = i;
        }
        if(!graphicsQueueIndexOpt.has_value())
            return;

        // Pick any GPU at first, then pick a discrete GPU over any other
        if(!pickedPDeviceOpt.has_value()
           || pDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            pickedPDeviceOpt = pDevice;
            graphicsQueueIndex = *graphicsQueueIndexOpt;
        }
    });

    if(!pickedPDeviceOpt.has_value())
        throw std::runtime_error("Couldn't find suitable physical device");
    auto pickedPDevice = *pickedPDeviceOpt;

    float queuePriorities = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo = {
        .queueFamilyIndex = graphicsQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriorities,
    };

    const char* requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = std::size(requiredExtensions),
        .ppEnabledExtensionNames = requiredExtensions,
        .pEnabledFeatures = nullptr,
    };

    return {
        .device = pickedPDevice.createDeviceUnique(deviceCreateInfo),
        .physicalDevice = pickedPDevice,
        .graphicsQueueIndex = graphicsQueueIndex,
    };
}

vk::UniqueRenderPass createRenderPass(const vk::UniqueDevice& device)
{
    vk::AttachmentDescription attachment = {
        .format = vk::Format::eB8G8R8A8Srgb,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp =
            vk::AttachmentLoadOp::eDontCare, // Not use since format does not define stencil
        .stencilStoreOp =
            vk::AttachmentStoreOp::eDontCare, // Not use since format does not define stencil
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };

    vk::AttachmentReference backbufferAttachment = {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::SubpassDescription subpass = {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &backbufferAttachment,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    vk::SubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dependencyFlags = vk::DependencyFlags(),
    };

    vk::RenderPassCreateInfo renderPassInfo = {
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    return device->createRenderPassUnique(renderPassInfo);
}

std::tuple<vk::UniquePipeline, vk::UniqueDescriptorSetLayout, vk::UniquePipelineLayout>
    createPipeline(
        const vk::UniqueDevice& device,
        const vk::UniqueRenderPass& renderPass,
        const vk::UniqueShaderModule& vertexShader,
        const vk::UniqueShaderModule& fragmentShader)
{
    vk::PipelineShaderStageCreateInfo vertexStageInfo = {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertexShader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };
    vk::PipelineShaderStageCreateInfo fragmentStageInfo = {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragmentShader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };

    vk::PipelineShaderStageCreateInfo stages[2] = {vertexStageInfo, fragmentStageInfo};

    vk::VertexInputBindingDescription vertexInputBindingDesc = {
        .binding = 0,
        .stride = 0,
        .inputRate = vk::VertexInputRate::eVertex,
    };

    std::array<vk::VertexInputAttributeDescription, 3> arr = {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = 0,
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = sizeof(float) * 3,
        },
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = sizeof(float) * (3 + 2),
        },
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDesc,
        .vertexAttributeDescriptionCount = arr.size(),
        .pVertexAttributeDescriptions = arr.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false,
    };

    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 800.0f, // TODO
        .height = 600.0f, // TODO
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent =
            {
                .width = 800,
                .height = 600,
            },
    };

    vk::PipelineViewportStateCreateInfo viewportInfo = {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizeInfo = {
        .depthClampEnable = false,
        .rasterizerDiscardEnable = true,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampleInfo = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    vk::PipelineColorBlendAttachmentState blendAttachmentInfo = {
        .blendEnable = false,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                          | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    std::array<float, 4> blendConstants = {0.0f, 0.0f, 0.0f, 0.0f};
    vk::PipelineColorBlendStateCreateInfo blendInfo = {
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eNoOp,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentInfo,
        .blendConstants = blendConstants,
    };

    vk::DescriptorSetLayoutBinding cameraPos = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutBinding viewProjection = {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutBinding transform = {
        .binding = 2,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    vk::DescriptorSetLayoutBinding bindings[3] = {cameraPos, viewProjection, transform};

    auto descriptorSetLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 3,
        .pBindings = bindings,
    });

    auto pipelineLayout = device->createPipelineLayoutUnique({
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    });

    vk::GraphicsPipelineCreateInfo pipelineInfo = {
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState = nullptr,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterizeInfo,
        .pMultisampleState = &multisampleInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &blendInfo,
        .pDynamicState = nullptr,
        .layout = *pipelineLayout,
        .renderPass = *renderPass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };
    auto pipeline = device->createGraphicsPipelineUnique(VK_NULL_HANDLE, pipelineInfo);

    return std::make_tuple(
        std::move(pipeline),
        std::move(descriptorSetLayout),
        std::move(pipelineLayout));
}

vk::UniqueSwapchainKHR createSwapchain(
    const vk::UniqueSurfaceKHR& surface,
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice,
    uint32_t queueFamilyIndex)
{
    auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    return device->createSwapchainKHRUnique({
        .surface = *surface,
        .minImageCount = 2,
        .imageFormat = vk::Format::eB8G8R8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = physicalDevice.getSurfaceCapabilitiesKHR(*surface).currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
        .oldSwapchain = VK_NULL_HANDLE,
    });
}

RendererVulkan::RendererVulkan(SDL_Window* windowHandle): currentFrame(0)
{
    // Without vulkan.hpp this would have to be done for every vkEnumerate function
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, nullptr);
    std::vector<const char*> sdlExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, sdlExtensions.data());

    this->instance = createInstance(sdlExtensions);
    this->debugCallback = initializeDebugCallback(instance);
    this->surface = createSurface(windowHandle, instance);
    {
        // Inner scope to avoid polluting function with temporary (and potentially moved!) variables
        auto [device, physicalDevice, graphicsQueueIndex] = createDevice(instance, surface);
        this->device = std::move(device);
        this->physicalDevice = std::move(physicalDevice);
        this->graphicsQueueIndex = graphicsQueueIndex;
    }
    this->graphicsQueue = device->getQueue(graphicsQueueIndex, 0);
    this->swapchain = createSwapchain(surface, device, physicalDevice, graphicsQueueIndex);
    this->renderPass = createRenderPass(device);
    for(auto& fence : queueDoneFences)
        fence = device->createFenceUnique({.flags = vk::FenceCreateFlagBits::eSignaled});
    for(auto& semaphore : imageAvailableSemaphores)
        semaphore = device->createSemaphoreUnique({});
    for(auto& semaphore : renderFinishedSemaphores)
        semaphore = device->createSemaphoreUnique({});

    this->samplerManager = std::make_unique<SamplerManagerVulkan>(this->device);
    this->bufferManager = std::make_unique<BufferManagerVulkan>(this->device, this->physicalDevice);
    this->textureManager = std::make_unique<TextureManagerVulkan>();
}

GraphicsRenderPass* RendererVulkan::CreateGraphicsRenderPass(
    const GraphicsRenderPassInfo& initialisationInfo)
{
    // Inner scopes prevents polluting function with random variables
    {
        auto vsDataOpt = FileUtils::readFile(initialisationInfo.vsPath);
        assert(vsDataOpt.has_value());
        auto vsData = vsDataOpt.value();
        // SPIR-V specifies 32-bit words, so cast vsData.data()
        shaderModules.push_back(device->createShaderModuleUnique(
            {.codeSize = vsData.size(), .pCode = (uint32_t*)vsData.data()}));
    }

    {
        // Note: called "fragment" from now on
        auto fsDataOpt = FileUtils::readFile(initialisationInfo.psPath);
        assert(fsDataOpt.has_value());
        auto fsData = fsDataOpt.value();
        shaderModules.push_back(device->createShaderModuleUnique(
            {.codeSize = fsData.size(), .pCode = (uint32_t*)fsData.data()}));
    }

    auto& vsModule = *(shaderModules.end() - 2);
    auto& fsModule = *(shaderModules.end() - 1);

    std::tie(this->pipeline, this->descriptorSetLayout, this->pipelineLayout) =
        createPipeline(this->device, this->renderPass, vsModule, fsModule);

    this->renderPasses.push_back(GraphicsRenderPassVulkan(
        vsModule,
        fsModule,
        initialisationInfo.objectBindings,
        initialisationInfo.globalBindings));
    return &this->renderPasses.back();
}

void RendererVulkan::DestroyGraphicsRenderPass(GraphicsRenderPass* pass) {}

Camera* RendererVulkan::CreateCamera(float minDepth, float maxDepth, float aspectRatio)
{
    assert(!this->cameraOpt);
    this->cameraOpt = CameraVulkan(*bufferManager, minDepth, maxDepth, aspectRatio);
    // The &* syntax is the best
    return &*this->cameraOpt;
}

void RendererVulkan::DestroyCamera(Camera* camera) {}

BufferManager* RendererVulkan::GetBufferManager()
{
    // There's no need to make sure this is valid since the constructor sets it up
    return bufferManager.get();
}

TextureManager* RendererVulkan::GetTextureManager()
{
    // There's no need to make sure this is valid since the constructor sets it up
    return textureManager.get();
}

SamplerManager* RendererVulkan::GetSamplerManager()
{
    // There's no need to make sure this is valid since the constructor sets it up
    return samplerManager.get();
}

void RendererVulkan::SetRenderPass(GraphicsRenderPass* toSet) {}

void RendererVulkan::SetCamera(Camera* toSet) {}

void RendererVulkan::SetLightBuffer(ResourceIndex lightBufferIndexToUse) {}

void RendererVulkan::PreRender()
{
    device->waitForFences(*queueDoneFences[currentFrame % BACKBUFFER_COUNT], true, UINT64_MAX);

    vk::Result res;
    std::tie(res, currentSwapchainImageIndex) = device->acquireNextImageKHR(
        *swapchain,
        UINT64_MAX,
        *imageAvailableSemaphores[currentFrame % BACKBUFFER_COUNT],
        VK_NULL_HANDLE);
    device->resetFences(*queueDoneFences[currentFrame % BACKBUFFER_COUNT]);
    assert(res == vk::Result::eSuccess);
}

void RendererVulkan::Render(const std::vector<RenderObject>& objectsToRender) {}

void RendererVulkan::Present()
{
    vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*imageAvailableSemaphores[currentFrame % BACKBUFFER_COUNT],
        .pWaitDstStageMask = &waitFlags,
        .commandBufferCount = 0,
        .pCommandBuffers = nullptr,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[currentFrame % BACKBUFFER_COUNT],
    };
    graphicsQueue.submit({submitInfo}, *queueDoneFences[currentFrame % BACKBUFFER_COUNT]);

    vk::PresentInfoKHR presentInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[currentFrame % BACKBUFFER_COUNT],
        .swapchainCount = 1,
        .pSwapchains = &*swapchain,
        .pImageIndices = &currentSwapchainImageIndex,
        .pResults = nullptr,
    };
    // This will _not_ return success if the window is resized
    assert(graphicsQueue.presentKHR(presentInfo) == vk::Result::eSuccess);

    currentFrame++;
}
