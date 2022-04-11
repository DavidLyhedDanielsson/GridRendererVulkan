#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "RenderObject.h"

enum class PipelineDataType
{
	NONE,
	TRANSFORM,
	VIEW_PROJECTION,
	CAMERA_POS,
	LIGHT,
	VERTEX,
	INDEX,
	DIFFUSE,
	SPECULAR,
	SAMPLER
};

enum class PipelineBindingType
{
	NONE,
	CONSTANT_BUFFER,
	SHADER_RESOURCE,
	UNORDERED_ACCESS
};

enum class PipelineShaderStage
{
	NONE,
	VS,
	PS
};

struct PipelineBinding
{
	PipelineDataType dataType = PipelineDataType::NONE;
	PipelineBindingType bindingType = PipelineBindingType::NONE;
	PipelineShaderStage shaderStage = PipelineShaderStage::NONE;
	std::uint8_t slotToBindTo = std::uint8_t(-1);
};

struct GraphicsRenderPassInfo
{
	std::string vsPath = "";
	std::string psPath = "";
	std::vector<PipelineBinding> objectBindings;
	std::vector<PipelineBinding> globalBindings;
};

class GraphicsRenderPass
{
protected:

public:
	GraphicsRenderPass() = default;
	virtual ~GraphicsRenderPass() = default;
	GraphicsRenderPass(const GraphicsRenderPass& other) = delete;
	GraphicsRenderPass& operator=(const GraphicsRenderPass& other) = delete;
	GraphicsRenderPass(GraphicsRenderPass&& other) = default;
	GraphicsRenderPass& operator=(GraphicsRenderPass&& other) = default;

	virtual void SetGlobalSampler(PipelineShaderStage shader,
		std::uint8_t slot, ResourceIndex index) = 0;
};