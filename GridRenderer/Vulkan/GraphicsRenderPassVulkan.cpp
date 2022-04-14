#include "GraphicsRenderPassVulkan.h"

#include <fstream>
#include <stdexcept>

GraphicsRenderPassVulkan::GraphicsRenderPassVulkan(
	const GraphicsRenderPassInfo& info) :
	objectBindings(info.objectBindings), globalBindings(info.globalBindings)
{
}

void GraphicsRenderPassVulkan::SetGlobalSampler(PipelineShaderStage shader,
	std::uint8_t slot, ResourceIndex index)
{
}
