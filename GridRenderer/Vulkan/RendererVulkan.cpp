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
    return false;
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

    // Use structured bindings if not using exceptions
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
    return vk::UniqueSurfaceKHR(surfaceRaw, *instance);
}

std::tuple<vk::UniqueDevice, vk::PhysicalDevice, uint32_t> createDevice(
    const vk::UniqueInstance& instance,
    const vk::UniqueSurfaceKHR& surface)
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
        .enabledExtensionCount = (uint32_t)std::size(requiredExtensions),
        .ppEnabledExtensionNames = requiredExtensions,
        .pEnabledFeatures = nullptr,
    };

    return std::make_tuple(
        pickedPDevice.createDeviceUnique(deviceCreateInfo),
        pickedPDevice,
        graphicsQueueIndex);
}

vk::UniqueRenderPass createRenderPass(const vk::UniqueDevice& device)
{
    vk::AttachmentDescription backBufferAttachmentDescription = {
        .format = vk::Format::eB8G8R8A8Srgb,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp =
            vk::AttachmentLoadOp::eDontCare, // Not used since format does not define stencil
        .stencilStoreOp =
            vk::AttachmentStoreOp::eDontCare, // Not used since format does not define stencil
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference backBufferAttachment = {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::AttachmentDescription depthbufferAttachmentDescription = {
        .format = vk::Format::eD24UnormS8Uint, // TODO: change to R32, requires
                                               // separateDepthStencilLayouts feature
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore, // TODO: store?
        .stencilLoadOp =
            vk::AttachmentLoadOp::eDontCare, // Not used since format does not define stencil
        .stencilStoreOp =
            vk::AttachmentStoreOp::eDontCare, // Not used since format does not define stencil
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal, // TODO: readonly?
    };
    vk::AttachmentReference depthbufferAttachment = {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::SubpassDescription subpass = {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &backBufferAttachment,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depthbufferAttachment,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    vk::SubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput
                        | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput
                        | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eNone,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite
                         | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dependencyFlags = vk::DependencyFlags(),
    };

    std::array<vk::AttachmentDescription, 2> attachmentDescriptions = {
        backBufferAttachmentDescription,
        depthbufferAttachmentDescription,
    };
    vk::RenderPassCreateInfo renderPassInfo = {
        .attachmentCount = (uint32_t)attachmentDescriptions.size(),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    return device->createRenderPassUnique(renderPassInfo);
}

std::tuple<vk::UniquePipeline, vk::UniquePipelineLayout> createPipeline(
    const vk::UniqueDevice& device,
    const vk::UniqueRenderPass& renderPass,
    const DescriptorSetLayouts& descriptorSetLayouts,
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

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false,
    };

    vk::Viewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 1280.0f, // TODO
        .height = 720.0f, // TODO
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
                .width = 1280, // TODO
                .height = 720, // TODO
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
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
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

    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = vk::CompareOp::eLessOrEqual,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
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

    auto allBindings = std::to_array({
        *descriptorSetLayouts.vertexIndex,
        *descriptorSetLayouts.transformBuffer,
        *descriptorSetLayouts.viewProjection,
        *descriptorSetLayouts.sampler,
        *descriptorSetLayouts.textures, // diffuse
        *descriptorSetLayouts.textures, // specular
        *descriptorSetLayouts.lights,
        *descriptorSetLayouts.cameraPosition,
    });
    auto pipelineLayout = device->createPipelineLayoutUnique({
        .setLayoutCount = (uint32_t)allBindings.size(),
        .pSetLayouts = allBindings.data(),
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
        .pDepthStencilState = &depthStencilInfo,
        .pColorBlendState = &blendInfo,
        .pDynamicState = nullptr,
        .layout = *pipelineLayout,
        .renderPass = *renderPass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };
    auto [error, pipeline] = device->createGraphicsPipelineUnique(VK_NULL_HANDLE, pipelineInfo);
    assert(error == vk::Result::eSuccess);

    return std::make_tuple(std::move(pipeline), std::move(pipelineLayout));
}

vk::UniqueSwapchainKHR createSwapchain(
    const vk::UniqueSurfaceKHR& surface,
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice)
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
        .queueFamilyIndexCount = 0, // Using EXCLUSIVE so this is not required
        .pQueueFamilyIndices = nullptr,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
        .oldSwapchain = VK_NULL_HANDLE,
    });
}

std::tuple<std::vector<vk::UniqueFramebuffer>, std::vector<vk::UniqueImageView>> createFramebuffers(
    const vk::UniqueDevice& device,
    const vk::UniqueSwapchainKHR& swapchain,
    const vk::UniqueRenderPass& renderPass,
    const vk::UniqueImageView& depthBuffer)
{
    std::vector<vk::Image> swapchainImages = device->getSwapchainImagesKHR(*swapchain);
    std::vector<vk::UniqueImageView> swapchainImageViews;
    std::vector<vk::UniqueFramebuffer> framebuffers;
    std::transform(
        entire_collection(swapchainImages),
        std::back_inserter(swapchainImageViews),
        [&](const vk::Image& image) {
            vk::ImageViewCreateInfo imageViewInfo = {
                .image = image,
                .viewType = vk::ImageViewType::e2D,
                .format = vk::Format::eB8G8R8A8Srgb,
                .components =
                    {
                        .r = vk::ComponentSwizzle::eIdentity,
                        .g = vk::ComponentSwizzle::eIdentity,
                        .b = vk::ComponentSwizzle::eIdentity,
                        .a = vk::ComponentSwizzle::eIdentity,
                    },
                .subresourceRange =
                    {
                        .aspectMask = vk::ImageAspectFlagBits::eColor,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            return device->createImageViewUnique(imageViewInfo);
        });
    std::transform(
        entire_collection(swapchainImageViews),
        std::back_inserter(framebuffers),
        [&](const vk::UniqueImageView& imageView) {
            std::array<vk::ImageView, 2> attachments = {
                *imageView,
                *depthBuffer,
            };
            vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = *renderPass,
                .attachmentCount = (uint32_t)attachments.size(),
                .pAttachments = attachments.data(),
                .width = 1280, // TODO
                .height = 720, // TODO
                .layers = 1,
            };

            return device->createFramebufferUnique(framebufferInfo);
        });

    return std::make_tuple(std::move(framebuffers), std::move(swapchainImageViews));
}

vk::UniqueDescriptorPool createDescriptorPool(const vk::UniqueDevice& device)
{
    std::array<vk::DescriptorPoolSize, 3> poolSizes = {
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 2,
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
        },
    };
    vk::DescriptorPoolCreateInfo poolInfo = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 10,
        .poolSizeCount = (uint32_t)poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    return device->createDescriptorPoolUnique(poolInfo);
}

DescriptorSetLayouts createDescriptorSetlayouts(const vk::UniqueDevice& device)
{
    vk::DescriptorSetLayoutBinding vertexBufferBinding = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutBinding indexBufferBinding = {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };
    vk::DescriptorSetLayoutBinding vertexIndexDescriptorInfo[2] = {
        vertexBufferBinding,
        indexBufferBinding,
    };
    auto vertexIndexLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 2,
        .pBindings = vertexIndexDescriptorInfo,
    });

    vk::DescriptorSetLayoutBinding transformBinding = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };
    auto transformLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &transformBinding,
    });

    vk::DescriptorSetLayoutBinding viewProjection = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };
    auto viewProjectionLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &viewProjection,
    });

    vk::DescriptorSetLayoutBinding sampler = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    auto samplerLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &sampler,
    });

    vk::DescriptorSetLayoutBinding textures = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    auto texturesLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &textures,
    });

    vk::DescriptorSetLayoutBinding lights = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    auto lightsLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &lights,
    });

    vk::DescriptorSetLayoutBinding cameraPosition = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };
    auto cameraPositionLayout = device->createDescriptorSetLayoutUnique({
        .bindingCount = 1,
        .pBindings = &cameraPosition,
    });

    return DescriptorSetLayouts{
        .vertexIndex = std::move(vertexIndexLayout),
        .transformBuffer = std::move(transformLayout),
        .viewProjection = std::move(viewProjectionLayout),
        .sampler = std::move(samplerLayout),
        .textures = std::move(texturesLayout),
        .lights = std::move(lightsLayout),
        .cameraPosition = std::move(cameraPositionLayout),
    };
}

std::tuple<vk::UniqueImage, vk::UniqueDeviceMemory, vk::UniqueImageView> createDepthBuffer(
    const vk::UniqueDevice& device,
    const vk::PhysicalDevice& physicalDevice,
    uint32_t queueFamilyIndex)
{
    vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eD24UnormS8Uint,
        .extent =
            {
                .width = 1280, // TODO
                .height = 720, // TODO
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queueFamilyIndex,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    auto depthBuffer = device->createImageUnique(imageInfo);

    vk::MemoryRequirements memoryRequirements = device->getImageMemoryRequirements(*depthBuffer);
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
    auto depthBufferMemory = device->allocateMemoryUnique(memoryInfo);
    device->bindImageMemory(*depthBuffer, *depthBufferMemory, 0);

    vk::ImageViewCreateInfo imageViewInfo = {
        .image = *depthBuffer,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eD24UnormS8Uint,
        .components =
            {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity,
            },
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eDepth,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    auto depthBufferView = device->createImageViewUnique(imageViewInfo);

    // The render pass will transition to the depth buffer layout

    return std::make_tuple(
        std::move(depthBuffer),
        std::move(depthBufferMemory),
        std::move(depthBufferView));
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
    std::tie(device, physicalDevice, graphicsQueueIndex) = createDevice(instance, surface);
    this->graphicsQueue = device->getQueue(graphicsQueueIndex, 0);
    this->swapchain = createSwapchain(surface, device, physicalDevice);
    this->renderPass = createRenderPass(device);
    std::tie(this->depthBuffer, this->depthBufferMemory, this->depthBufferView) =
        createDepthBuffer(device, physicalDevice, graphicsQueueIndex);
    std::tie(this->framebuffers, this->backBufferImageViews) =
        createFramebuffers(device, swapchain, renderPass, depthBufferView);
    for(auto& fence : queueDoneFences)
        fence = device->createFenceUnique({.flags = vk::FenceCreateFlagBits::eSignaled});
    for(auto& semaphore : imageAvailableSemaphores)
        semaphore = device->createSemaphoreUnique({});
    for(auto& semaphore : renderFinishedSemaphores)
        semaphore = device->createSemaphoreUnique({});

    this->descriptorSetLayouts = createDescriptorSetlayouts(device);

    this->samplerManager =
        std::make_unique<SamplerManagerVulkan>(this->device, descriptorSetLayouts.sampler);
    this->bufferManager = std::make_unique<BufferManagerVulkan>(this->device, this->physicalDevice);
    this->textureManager = std::make_unique<TextureManagerVulkan>(
        this->device,
        this->physicalDevice,
        this->graphicsQueue,
        this->graphicsQueueIndex,
        descriptorSetLayouts.textures);

    this->commandPool = device->createCommandPoolUnique({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueIndex,
    });
    this->commandBuffers = device->allocateCommandBuffersUnique({
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 3,
    });
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

    std::tie(this->pipeline, this->pipelineLayout) = createPipeline(
        this->device,
        this->renderPass,
        this->descriptorSetLayouts,
        vsModule,
        fsModule);

    this->descriptorPool = createDescriptorPool(device);
    vk::DescriptorSetAllocateInfo descSetInfo = {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayouts.vertexIndex,
    };
    this->vertexIndexDescriptorSet =
        std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);

    for(uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
    {
        descSetInfo = {
            .descriptorPool = *descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayouts.transformBuffer,
        };
        this->transformDescriptorSets[i] =
            std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);
    }

    descSetInfo = {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayouts.viewProjection,
    };
    this->viewProjectionDescriptorSet =
        std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);

    descSetInfo = {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayouts.lights,
    };
    this->lightBufferDescriptorSet =
        std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);

    descSetInfo = {
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayouts.cameraPosition,
    };
    this->cameraPositionDescriptorSet =
        std::move(device->allocateDescriptorSetsUnique(descSetInfo)[0]);

    this->renderPasses.push_back(GraphicsRenderPassVulkan(
        vsModule,
        fsModule,
        initialisationInfo.objectBindings,
        initialisationInfo.globalBindings));
    return &this->renderPasses.back();
}

void RendererVulkan::DestroyGraphicsRenderPass(GraphicsRenderPass* pass)
{
    device->waitIdle();
}

Camera* RendererVulkan::CreateCamera(float minDepth, float maxDepth, float aspectRatio)
{
    assert(!this->cameraOpt);
    this->cameraOpt = CameraVulkan(*bufferManager, minDepth, maxDepth, aspectRatio);

    CameraVulkan& camera = this->cameraOpt.value();
    auto viewProj = camera.GetViewProjMatrix();
    cameraBufferIndex = bufferManager->AddBuffer(
        &viewProj,
        sizeof(float),
        16,
        PerFrameWritePattern::NEVER,
        PerFrameWritePattern::NEVER,
        0);

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

void RendererVulkan::SetLightBuffer(ResourceIndex lightBufferIndexToUse)
{
    this->lightBufferIndex = lightBufferIndexToUse;

    const auto& buffer = bufferManager->GetBuffer(lightBufferIndex);
    vk::DescriptorBufferInfo bufferInfo = {
        .buffer = bufferManager->GetBackingBuffer(lightBufferIndex),
        .offset = buffer.backingBufferOffset,
        .range = buffer.sizeWithoutPadding,
    };
    vk::WriteDescriptorSet bufferDescriptor = {
        .dstSet = *lightBufferDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pImageInfo = nullptr,
        .pBufferInfo = &bufferInfo,
        .pTexelBufferView = nullptr,
    };
    device->updateDescriptorSets(1, &bufferDescriptor, 0, nullptr);
}

void RendererVulkan::PreRender()
{
    assert(
        device->waitForFences(*queueDoneFences[currentFrame % BACKBUFFER_COUNT], true, UINT64_MAX)
        == vk::Result::eSuccess);

    vk::Result res;
    std::tie(res, currentSwapchainImageIndex) = device->acquireNextImageKHR(
        *swapchain,
        UINT64_MAX,
        *imageAvailableSemaphores[currentFrame % BACKBUFFER_COUNT],
        VK_NULL_HANDLE);
    device->resetFences(*queueDoneFences[currentFrame % BACKBUFFER_COUNT]);
    assert(res == vk::Result::eSuccess);

    glm::mat4 viewProj = cameraOpt->GetViewProjMatrix();
    bufferManager->UpdateBuffer(cameraBufferIndex, &viewProj);
    glm::vec3 cameraPosition = cameraOpt->GetPosition();
    bufferManager->UpdateBuffer(cameraOpt->GetCameraPositionBufferIndex(), &cameraPosition);

    commandBuffers[currentFrame % BACKBUFFER_COUNT]->reset();
    commandBuffers[currentFrame % BACKBUFFER_COUNT]->begin(
        {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    commandBuffers[currentFrame % BACKBUFFER_COUNT]->bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        *pipeline);

    commandBuffers[currentFrame % BACKBUFFER_COUNT]->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        6,
        1,
        &*lightBufferDescriptorSet,
        0,
        nullptr);
    commandBuffers[currentFrame % BACKBUFFER_COUNT]->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        7,
        1,
        &*cameraPositionDescriptorSet,
        0,
        nullptr);
}

void RendererVulkan::Render(const std::vector<RenderObject>& objectsToRender)
{
    static bool once = false;
    if(!once)
    {
        const auto& renderObject = objectsToRender[0];

        auto vertexBuffer = bufferManager->GetBuffer(renderObject.GetMesh().GetVertexBuffer());
        vk::DescriptorBufferInfo vertexBufferInfo = {
            .buffer = bufferManager->GetBackingBuffer(renderObject.GetMesh().GetVertexBuffer()),
            .offset = vertexBuffer.backingBufferOffset,
            .range = vertexBuffer.sizeWithoutPadding,
        };
        vk::WriteDescriptorSet vertexWriteDescriptor = {
            .dstSet = *vertexIndexDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &vertexBufferInfo,
            .pTexelBufferView = nullptr,
        };

        auto indexBuffer = bufferManager->GetBuffer(renderObject.GetMesh().GetIndexBuffer());
        vk::DescriptorBufferInfo indexBufferInfo = {
            .buffer = bufferManager->GetBackingBuffer(renderObject.GetMesh().GetIndexBuffer()),
            .offset = indexBuffer.backingBufferOffset,
            .range = indexBuffer.sizeWithoutPadding,
        };
        vk::WriteDescriptorSet indexWriteDescriptor = {
            .dstSet = *vertexIndexDescriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &indexBufferInfo,
            .pTexelBufferView = nullptr,
        };

        vk::WriteDescriptorSet descriptorSets[2] = {vertexWriteDescriptor, indexWriteDescriptor};
        device->updateDescriptorSets(2, descriptorSets, 0, nullptr);

        // Transform updates every frame
        for(uint32_t i = 0; i < BACKBUFFER_COUNT; ++i)
        {
            vk::DescriptorBufferInfo roundRobinBufferInfo = {
                .buffer = bufferManager->GetRoundRobinBuffer(),
                .offset = bufferManager->GetRoundRobinChunkSize() * i,
                .range = bufferManager->GetRoundRobinChunkSize(),
            };
            vk::WriteDescriptorSet transformWriteDescriptor = {
                .dstSet = *transformDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pImageInfo = nullptr,
                .pBufferInfo = &roundRobinBufferInfo,
                .pTexelBufferView = nullptr,
            };
            device->updateDescriptorSets(1, &transformWriteDescriptor, 0, nullptr);
        }

        auto viewProjectionBuffer = bufferManager->GetBuffer(cameraBufferIndex);
        vk::DescriptorBufferInfo viewProjectionBufferInfo = {
            .buffer = bufferManager->GetBackingBuffer(cameraBufferIndex),
            .offset = viewProjectionBuffer.backingBufferOffset,
            .range = viewProjectionBuffer.sizeWithoutPadding,
        };
        vk::WriteDescriptorSet viewProjectionWriteDescriptor = {
            .dstSet = *viewProjectionDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &viewProjectionBufferInfo,
            .pTexelBufferView = nullptr,
        };
        device->updateDescriptorSets(1, &viewProjectionWriteDescriptor, 0, nullptr);

        const CameraVulkan& camera = *this->cameraOpt;
        const auto& buffer = bufferManager->GetBuffer(camera.GetCameraPositionBufferIndex());
        vk::DescriptorBufferInfo bufferInfo = {
            .buffer = bufferManager->GetBackingBuffer(camera.GetCameraPositionBufferIndex()),
            .offset = buffer.backingBufferOffset,
            .range = buffer.sizeWithoutPadding,
        };
        vk::WriteDescriptorSet bufferDescriptor = {
            .dstSet = *cameraPositionDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = nullptr,
        };
        device->updateDescriptorSets(1, &bufferDescriptor, 0, nullptr);

        once = true;
    }
    const vk::CommandBuffer& commandBuffer = *commandBuffers[currentFrame % BACKBUFFER_COUNT];

    std::array<vk::DescriptorSet, 3> descriptorSets = {
        *vertexIndexDescriptorSet,
        *transformDescriptorSets[currentFrame % BACKBUFFER_COUNT],
        *viewProjectionDescriptorSet,
    };
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        (uint32_t)descriptorSets.size(),
        descriptorSets.data(),
        0,
        nullptr);

    std::vector<vk::BufferCopy> copyInfo;
    uint32_t copyOffset = 0;
    for(size_t i = 0; i < objectsToRender.size(); ++i)
    {
        auto transformBuffer =
            bufferManager->GetBuffer(objectsToRender[i].GetTransformBufferIndex());
        copyInfo.push_back(vk::BufferCopy{
            .srcOffset = transformBuffer.backingBufferOffset,
            .dstOffset = bufferManager->GetRoundRobinChunkSize() * (currentFrame % BACKBUFFER_COUNT)
                         + copyOffset,
            .size = transformBuffer.sizeWithoutPadding,
        });

        copyOffset += transformBuffer.sizeWithPadding;

        commandBuffer.copyBuffer(
            bufferManager->GetBackingBuffer(objectsToRender[i].GetTransformBufferIndex()),
            bufferManager->GetRoundRobinBuffer(),
            copyInfo);
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eVertexInput,
            vk::DependencyFlagBits::eByRegion,
            0,
            nullptr,
            0,
            nullptr,
            0,
            nullptr);
    }

    std::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].color = vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    vk::RenderPassBeginInfo info = {
        .renderPass = *renderPass,
        .framebuffer = *framebuffers[currentSwapchainImageIndex],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {1280, 720} // TODO
            },
        .clearValueCount = (uint32_t)clearValues.size(),
        .pClearValues = clearValues.data(),
    };
    commandBuffer.beginRenderPass(info, vk::SubpassContents::eInline);
    uint32_t startIndex = 0;
    for(uint32_t endIndex = 1; endIndex < objectsToRender.size(); endIndex++)
    {
        if(objectsToRender[endIndex].GetSurfaceProperty().GetDiffuseTexture()
               != objectsToRender[startIndex].GetSurfaceProperty().GetDiffuseTexture()
           || endIndex + 1 == objectsToRender.size())
        {
            const RenderObject& renderObject = objectsToRender[startIndex];

            auto descriptorSets = std::to_array<vk::DescriptorSet>({
                // samplerManager->GetDescriptorSet(renderObject.GetSurfaceProperty().GetSampler()),
                samplerManager->GetDescriptorSet(0), // TODO
                textureManager->GetDescriptorSet(
                    renderObject.GetSurfaceProperty().GetDiffuseTexture()),
                textureManager->GetDescriptorSet(
                    renderObject.GetSurfaceProperty().GetSpecularTexture()),
            });
            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipelineLayout,
                3,
                (uint32_t)descriptorSets.size(),
                descriptorSets.data(),
                0,
                nullptr);

            commandBuffer.draw(
                bufferManager->GetElementCount(renderObject.GetMesh().GetIndexBuffer()),
                endIndex - startIndex,
                0,
                startIndex);

            startIndex = endIndex;
        }
    }
    commandBuffer.endRenderPass();
}

void RendererVulkan::Present()
{
    commandBuffers[currentFrame % BACKBUFFER_COUNT]->end();

    vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*imageAvailableSemaphores[currentFrame % BACKBUFFER_COUNT],
        .pWaitDstStageMask = &waitFlags,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[currentFrame % BACKBUFFER_COUNT],
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
