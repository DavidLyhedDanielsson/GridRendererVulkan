#include "SamplerManagerVulkan.h"

SamplerManagerVulkan::SamplerManagerVulkan(vk::UniqueDevice& device): device(device) {}

ResourceIndex SamplerManagerVulkan::CreateSampler(SamplerType type, AddressMode adressMode)
{
    return 0;
}
