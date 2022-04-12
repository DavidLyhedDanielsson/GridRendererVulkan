#include "RendererVulkan.h"

#include <map>
#include <optional>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

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

    throw std::runtime_error(std::string(pCallbackData->pMessage));
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
        .graphicsQueueIndex = graphicsQueueIndex,
    };
}

RendererVulkan::RendererVulkan(SDL_Window* windowHandle)
{
    // Without vulkan.hpp this would have to be done for every vkEnumerate function
    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, nullptr);
    std::vector<const char*> sdlExtensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(windowHandle, &extensionCount, sdlExtensions.data());

    this->instance = createInstance(sdlExtensions);
    this->debugCallback = initializeDebugCallback(instance);
    this->surface = createSurface(windowHandle, instance);
    auto [device, graphicsQueueIndex] = createDevice(instance, surface);
    this->device = std::move(device);
    this->graphicsQueueIndex = graphicsQueueIndex;
}

GraphicsRenderPass* RendererVulkan::CreateGraphicsRenderPass(
    const GraphicsRenderPassInfo& intialisationInfo)
{
    return nullptr;
}

void RendererVulkan::DestroyGraphicsRenderPass(GraphicsRenderPass* pass) {}

Camera* RendererVulkan::CreateCamera(float minDepth, float maxDepth, float aspectRatio)
{
    return nullptr;
}

void RendererVulkan::DestroyCamera(Camera* camera) {}

BufferManager* RendererVulkan::GetBufferManager()
{
    return nullptr;
}

TextureManager* RendererVulkan::GetTextureManager()
{
    return nullptr;
}

SamplerManager* RendererVulkan::GetSamplerManager()
{
    return nullptr;
}

void RendererVulkan::SetRenderPass(GraphicsRenderPass* toSet) {}

void RendererVulkan::SetCamera(Camera* toSet) {}

void RendererVulkan::SetLightBuffer(ResourceIndex lightBufferIndexToUse) {}

void RendererVulkan::PreRender() {}

void RendererVulkan::Render(const std::vector<RenderObject>& objectsToRender) {}

void RendererVulkan::Present() {}
