#include "Vulkan/Graphics/Pipeline.hpp"

#include "Error.hpp"
#include "Types.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"
#include "Vulkan/Types.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

WIND_NODISCARD auto CreatePipeline(const vk::raii::Device &Device, FGraphicsConfig Cfg) WIND_NOEXCEPT
    -> TWindResult<FGraphicsPipeline>
{
	vk::GraphicsPipelineCreateInfo GpCreateInfo{};

	for (const auto &Shader : Cfg.Shader)
	{
		spdlog::info("config shader module = {}", reinterpret_cast<uintptr_t>(Shader.Module));
	}

	auto PipelineCreateInfo = ToVk(Cfg);
	vk::PipelineVertexInputStateCreateInfo VertexInput{};
	VertexInput.vertexAttributeDescriptionCount = static_cast<uint32>(PipelineCreateInfo.Attributes.size());
	VertexInput.pVertexAttributeDescriptions = PipelineCreateInfo.Attributes.data();
	VertexInput.vertexBindingDescriptionCount = static_cast<uint32>(PipelineCreateInfo.Bindings.size());
	VertexInput.pVertexBindingDescriptions = PipelineCreateInfo.Bindings.data();

	vk::PipelineColorBlendStateCreateInfo ColorBlend{};
	ColorBlend.attachmentCount = 1;
	ColorBlend.pAttachments = &PipelineCreateInfo.ColorBlendAttachment;

	std::array DynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo DynamicStateInfo{};
	DynamicStateInfo.dynamicStateCount = DynamicStates.size();
	DynamicStateInfo.pDynamicStates = DynamicStates.data();

	vk::PipelineViewportStateCreateInfo ViewportState{};
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;

	vk::Format ColorAttachmentFormat = ToVk(Cfg.ColorFormat);

	vk::PipelineRenderingCreateInfo RenderingInfo{};
	RenderingInfo.colorAttachmentCount = 1;
	RenderingInfo.pColorAttachmentFormats = &ColorAttachmentFormat;
	RenderingInfo.depthAttachmentFormat = ToVk(Cfg.DepthFormat);
	RenderingInfo.stencilAttachmentFormat = vk::Format::eUndefined;

	std::vector<vk::PushConstantRange> PushConstantRanges;
	PushConstantRanges.reserve(Cfg.PushConstants.size());

	for (auto &PushConstant : Cfg.PushConstants)
	{
		PushConstantRanges.push_back(ToVk(PushConstant));
	}

	vk::PipelineLayoutCreateInfo LayoutInfo{};
	LayoutInfo.pushConstantRangeCount = static_cast<uint32>(PushConstantRanges.size());
	LayoutInfo.pPushConstantRanges = PushConstantRanges.data();

	if (Cfg.DescriptorSetLayout)
	{
		// since we are using bindless descriptor set we only need one layout
		LayoutInfo.setLayoutCount = 1;
		LayoutInfo.pSetLayouts = &*Cfg.DescriptorSetLayout;
	}

	auto Layout = WIND_TRY(Device.createPipelineLayout(LayoutInfo), ErrorCode::FailedToCreatePipelineLayout);

	GpCreateInfo.pNext = &RenderingInfo;
	GpCreateInfo.stageCount = PipelineCreateInfo.ShaderStages.size();
	GpCreateInfo.pStages = PipelineCreateInfo.ShaderStages.data();
	GpCreateInfo.pInputAssemblyState = &PipelineCreateInfo.InputAssembly;
	GpCreateInfo.pViewportState = &ViewportState;
	GpCreateInfo.pVertexInputState = &VertexInput;
	GpCreateInfo.pTessellationState = nullptr;
	GpCreateInfo.pRasterizationState = &PipelineCreateInfo.Rasterization;
	GpCreateInfo.pMultisampleState = &PipelineCreateInfo.Multisample;
	GpCreateInfo.pDepthStencilState = &PipelineCreateInfo.DepthStencil;
	GpCreateInfo.pColorBlendState = &ColorBlend;
	GpCreateInfo.pDynamicState = &DynamicStateInfo;
	GpCreateInfo.renderPass = nullptr;
	GpCreateInfo.layout = Layout;

	auto GraphicsPipeline =
	    WIND_TRY(Device.createGraphicsPipeline(nullptr, GpCreateInfo), ErrorCode::FailedToCreatePipeline);

	return FGraphicsPipeline{.PipelineLayout = std::move(Layout), .GraphicsPipeline = std::move(GraphicsPipeline)};
}
