#include "GraphicsRenderPassVulkan.h"

#include <fstream>
#include <stdexcept>

GraphicsRenderPassVulkan::GraphicsRenderPassVulkan(
    const vk::UniqueShaderModule& vsShader,
    const vk::UniqueShaderModule& fsShader,
    std::vector<PipelineBinding> objectBindings,
    std::vector<PipelineBinding> globalBindings)
    : vsShader(vsShader)
    , fsShader(fsShader)
    , objectBindings(objectBindings)
    , globalBindings(globalBindings)
{
}

void GraphicsRenderPassVulkan::SetGlobalSampler(
    PipelineShaderStage shader,
    std::uint8_t slot,
    ResourceIndex index)
{
}
