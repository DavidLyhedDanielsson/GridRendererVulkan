#pragma once

#include <array>
#include <vulkan/vulkan_raii.hpp>

#include "../GraphicsRenderPass.h"

class GraphicsRenderPassVulkan: public GraphicsRenderPass
{
  private:
    const vk::UniqueShaderModule& vsShader;
    const vk::UniqueShaderModule& fsShader;
    std::vector<PipelineBinding> objectBindings;
    std::vector<PipelineBinding> globalBindings;

  public:
    GraphicsRenderPassVulkan(
        const vk::UniqueShaderModule& vsShader,
        const vk::UniqueShaderModule& fsShader,
        std::vector<PipelineBinding> objectBindings,
        std::vector<PipelineBinding> globalBindings);
    GraphicsRenderPassVulkan(const GraphicsRenderPassVulkan& other) = delete;
    GraphicsRenderPassVulkan& operator=(const GraphicsRenderPassVulkan& other) = delete;
    GraphicsRenderPassVulkan(GraphicsRenderPassVulkan&& other) = default;
    GraphicsRenderPassVulkan& operator=(GraphicsRenderPassVulkan&& other) = default;

    const std::vector<PipelineBinding>& GetObjectBindings();
    const std::vector<PipelineBinding>& GetGlobalBindings();

    void SetGlobalSampler(PipelineShaderStage shader, std::uint8_t slot, ResourceIndex index)
        override;
};
