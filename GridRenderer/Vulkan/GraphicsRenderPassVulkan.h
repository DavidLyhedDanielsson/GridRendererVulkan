#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <array>

#include "../GraphicsRenderPass.h"

class GraphicsRenderPassVulkan : public GraphicsRenderPass
{
private:
	std::vector<PipelineBinding> objectBindings;
	std::vector<PipelineBinding> globalBindings;

public:
	GraphicsRenderPassVulkan(const GraphicsRenderPassInfo& info);
	GraphicsRenderPassVulkan(const GraphicsRenderPassVulkan& other) = delete;
	GraphicsRenderPassVulkan& operator=(const GraphicsRenderPassVulkan& other) = delete;
	GraphicsRenderPassVulkan(GraphicsRenderPassVulkan&& other) = default;
	GraphicsRenderPassVulkan& operator=(GraphicsRenderPassVulkan&& other) = default;

	const std::vector<PipelineBinding>& GetObjectBindings();
	const std::vector<PipelineBinding>& GetGlobalBindings();

	void SetGlobalSampler(PipelineShaderStage shader, 
		std::uint8_t slot, ResourceIndex index) override;
};
