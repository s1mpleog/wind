#pragma once

#include "Config.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"

#include <span>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

constexpr auto ToVk(EVulkanVersion Value) WIND_NOEXCEPT -> uint32_t
{
	switch (Value)
	{
	case EVulkanVersion::VERSION13:
		return vk::ApiVersion13;

	case EVulkanVersion::VERSION14:
		return vk::ApiVersion14;
	}

	std::unreachable();
}

constexpr auto ToVk(EDebugMessageSeverity Value) WIND_NOEXCEPT -> vk::DebugUtilsMessageSeverityFlagBitsEXT
{
	return static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(static_cast<uint32>(Value));
}

constexpr auto ToVk(EDebugMessageType Value) WIND_NOEXCEPT -> vk::DebugUtilsMessageTypeFlagsEXT
{
	return static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(static_cast<uint32>(Value));
}

auto ToVk(EPrimitiveTopology Topology) WIND_NOEXCEPT -> vk::PrimitiveTopology;
auto ToVk(ECullMode CullMode) WIND_NOEXCEPT -> vk::CullModeFlagBits;
auto ToVk(EPolygonMode PolygonMode) WIND_NOEXCEPT -> vk::PolygonMode;
auto ToVk(EFrontFace FrontFace) WIND_NOEXCEPT -> vk::FrontFace;
auto ToVk(EFormat Format) WIND_NOEXCEPT -> vk::Format;
auto ToVk(EVertexFormat Format) WIND_NOEXCEPT -> vk::Format;
auto ToVk(EVertexInputRate InputRate) WIND_NOEXCEPT -> vk::VertexInputRate;
auto ToVk(ECompareOp CompareOp) WIND_NOEXCEPT -> vk::CompareOp;
auto ToVk(EShaderStage Stage) WIND_NOEXCEPT -> vk::ShaderStageFlagBits;
auto ToVk(const FVertexAttribute &Attribute) WIND_NOEXCEPT -> vk::VertexInputAttributeDescription;
auto ToVk(const FVertexBinding &Binding) WIND_NOEXCEPT -> vk::VertexInputBindingDescription;
auto ToVk(const FVertexInputState &VertexInput) WIND_NOEXCEPT
    -> std::pair<std::vector<vk::VertexInputAttributeDescription>, std::vector<vk::VertexInputBindingDescription>>;
auto ToVk(const FRasterizationState &Rasterization) WIND_NOEXCEPT -> vk::PipelineRasterizationStateCreateInfo;
auto ToVk(const FInputAssemblyState &InputAssembly) WIND_NOEXCEPT -> vk::PipelineInputAssemblyStateCreateInfo;
auto ToVk(const FDepthStencilState &DepthStencil) WIND_NOEXCEPT -> vk::PipelineDepthStencilStateCreateInfo;
auto ToVk(EBlendFactor BlendFactor) WIND_NOEXCEPT -> vk::BlendFactor;
auto ToVk(EBlendOp BlendOp) WIND_NOEXCEPT -> vk::BlendOp;
auto ToVk(const FColorBlendState &ColorBlend) WIND_NOEXCEPT -> vk::PipelineColorBlendAttachmentState;

// Implementation

WIND_INLINE auto ToVk(EPrimitiveTopology Topology) WIND_NOEXCEPT -> vk::PrimitiveTopology
{
	switch (Topology)
	{
	case EPrimitiveTopology::PointList:
		return vk::PrimitiveTopology::ePointList;
	case EPrimitiveTopology::LineList:
		return vk::PrimitiveTopology::eLineList;
	case EPrimitiveTopology::LineStrip:
		return vk::PrimitiveTopology::eLineStrip;
	case EPrimitiveTopology::TriangleList:
		return vk::PrimitiveTopology::eTriangleList;
	case EPrimitiveTopology::TriangleStrip:
		return vk::PrimitiveTopology::eTriangleStrip;
	case EPrimitiveTopology::TriangleFan:
		return vk::PrimitiveTopology::eTriangleFan;
	default:
		return vk::PrimitiveTopology::eTriangleList;
	}
}

WIND_INLINE auto ToVk(ECullMode CullMode) WIND_NOEXCEPT -> vk::CullModeFlagBits
{
	switch (CullMode)
	{
	case ECullMode::Back:
		return vk::CullModeFlagBits::eBack;
	case ECullMode::FontAndBack:
		return vk::CullModeFlagBits::eFrontAndBack;
	case ECullMode::None:
		return vk::CullModeFlagBits::eNone;
	default:
		return vk::CullModeFlagBits::eNone;
	}
}

WIND_INLINE auto ToVk(EPolygonMode PolygonMode) WIND_NOEXCEPT -> vk::PolygonMode
{
	switch (PolygonMode)
	{
	case EPolygonMode::Fill:
		return vk::PolygonMode::eFill;
	case EPolygonMode::Line:
		return vk::PolygonMode::eLine;
	case EPolygonMode::Point:
		return vk::PolygonMode::ePoint;
	default:
		return vk::PolygonMode::eFill;
	}
}

WIND_INLINE auto ToVk(EFrontFace FrontFace) WIND_NOEXCEPT -> vk::FrontFace
{
	switch (FrontFace)
	{
	case EFrontFace::CounterClockwise:
		return vk::FrontFace::eCounterClockwise;
	case EFrontFace::ClockWise:
		return vk::FrontFace::eClockwise;
	default:
		return vk::FrontFace::eCounterClockwise;
	}
}

WIND_INLINE auto ToVk(EFormat Format) WIND_NOEXCEPT -> vk::Format
{
	switch (Format)
	{
	case EFormat::Undefined:
		return vk::Format::eUndefined;

	// 8-bit formats
	case EFormat::R8Unorm:
		return vk::Format::eR8Unorm;
	case EFormat::R8Snorm:
		return vk::Format::eR8Snorm;
	case EFormat::R8Uint:
		return vk::Format::eR8Uint;
	case EFormat::R8Sint:
		return vk::Format::eR8Sint;
	case EFormat::R8Srgb:
		return vk::Format::eR8Srgb;

	case EFormat::RG8Unorm:
		return vk::Format::eR8G8Unorm;
	case EFormat::RG8Snorm:
		return vk::Format::eR8G8Snorm;
	case EFormat::RG8Uint:
		return vk::Format::eR8G8Uint;
	case EFormat::RG8Sint:
		return vk::Format::eR8G8Sint;
	case EFormat::RG8Srgb:
		return vk::Format::eR8G8Srgb;

	case EFormat::RgbA8Unorm:
		return vk::Format::eR8G8B8A8Unorm;
	case EFormat::RgbA8Snorm:
		return vk::Format::eR8G8B8A8Snorm;
	case EFormat::RgbA8Uint:
		return vk::Format::eR8G8B8A8Uint;
	case EFormat::RgbA8Sint:
		return vk::Format::eR8G8B8A8Sint;
	case EFormat::RgbA8Srgb:
		return vk::Format::eR8G8B8A8Srgb;

	case EFormat::BgrA8Unorm:
		return vk::Format::eB8G8R8A8Unorm;
	case EFormat::BgrA8Srgb:
		return vk::Format::eB8G8R8A8Srgb;

	// Packed formats
	case EFormat::RgB565UnormPacK16:
		return vk::Format::eR5G6B5UnormPack16;
	case EFormat::RgbA4UnormPacK16:
		return vk::Format::eR4G4B4A4UnormPack16;
	case EFormat::RgB5A1UnormPacK16:
		return vk::Format::eR5G5B5A1UnormPack16;
	case EFormat::A2B10G10R10UnormPacK32:
		return vk::Format::eA2B10G10R10UnormPack32;
	case EFormat::A2B10G10R10UintPacK32:
		return vk::Format::eA2B10G10R10UintPack32;
	case EFormat::A2R10G10B10UnormPacK32:
		return vk::Format::eA2R10G10B10UnormPack32;
	case EFormat::B10G11R11UfloatPacK32:
		return vk::Format::eB10G11R11UfloatPack32;
	case EFormat::E5B9G9R9UfloatPacK32:
		return vk::Format::eE5B9G9R9UfloatPack32;

	// 16-bit formats
	case EFormat::R16Unorm:
		return vk::Format::eR16Unorm;
	case EFormat::R16Snorm:
		return vk::Format::eR16Snorm;
	case EFormat::R16Uint:
		return vk::Format::eR16Uint;
	case EFormat::R16Sint:
		return vk::Format::eR16Sint;
	case EFormat::R16Float:
		return vk::Format::eR16Sfloat;

	case EFormat::RG16Unorm:
		return vk::Format::eR16G16Unorm;
	case EFormat::RG16Snorm:
		return vk::Format::eR16G16Snorm;
	case EFormat::RG16Uint:
		return vk::Format::eR16G16Uint;
	case EFormat::RG16Sint:
		return vk::Format::eR16G16Sint;
	case EFormat::RG16Float:
		return vk::Format::eR16G16Sfloat;

	case EFormat::RgbA16Unorm:
		return vk::Format::eR16G16B16A16Unorm;
	case EFormat::RgbA16Snorm:
		return vk::Format::eR16G16B16A16Snorm;
	case EFormat::RgbA16Uint:
		return vk::Format::eR16G16B16A16Uint;
	case EFormat::RgbA16Sint:
		return vk::Format::eR16G16B16A16Sint;
	case EFormat::RgbA16Float:
		return vk::Format::eR16G16B16A16Sfloat;

	// 32-bit formats
	case EFormat::R32Uint:
		return vk::Format::eR32Uint;
	case EFormat::R32Sint:
		return vk::Format::eR32Sint;
	case EFormat::R32Float:
		return vk::Format::eR32Sfloat;

	case EFormat::RG32Uint:
		return vk::Format::eR32G32Uint;
	case EFormat::RG32Sint:
		return vk::Format::eR32G32Sint;
	case EFormat::RG32Float:
		return vk::Format::eR32G32Sfloat;

	case EFormat::RgB32Uint:
		return vk::Format::eR32G32B32Uint;
	case EFormat::RgB32Sint:
		return vk::Format::eR32G32B32Sint;
	case EFormat::RgB32Float:
		return vk::Format::eR32G32B32Sfloat;

	case EFormat::RgbA32Uint:
		return vk::Format::eR32G32B32A32Uint;
	case EFormat::RgbA32Sint:
		return vk::Format::eR32G32B32A32Sint;
	case EFormat::RgbA32Float:
		return vk::Format::eR32G32B32A32Sfloat;

	// Depth/Stencil formats
	case EFormat::D16Unorm:
		return vk::Format::eD16Unorm;
	case EFormat::X8D24UnormPacK32:
		return vk::Format::eX8D24UnormPack32;
	case EFormat::D32Float:
		return vk::Format::eD32Sfloat;
	case EFormat::S8Uint:
		return vk::Format::eS8Uint;
	case EFormat::D16UnormS8Uint:
		return vk::Format::eD16UnormS8Uint;
	case EFormat::D24UnormS8Uint:
		return vk::Format::eD24UnormS8Uint;
	case EFormat::D32FloatS8Uint:
		return vk::Format::eD32SfloatS8Uint;

	// BC compressed formats
	case EFormat::BC1RgbUnormBlock:
		return vk::Format::eBc1RgbUnormBlock;
	case EFormat::BC1RgbSrgbBlock:
		return vk::Format::eBc1RgbSrgbBlock;
	case EFormat::BC1RgbaUnormBlock:
		return vk::Format::eBc1RgbaUnormBlock;
	case EFormat::BC1RgbaSrgbBlock:
		return vk::Format::eBc1RgbaSrgbBlock;
	case EFormat::BC2UnormBlock:
		return vk::Format::eBc2UnormBlock;
	case EFormat::BC2SrgbBlock:
		return vk::Format::eBc2SrgbBlock;
	case EFormat::BC3UnormBlock:
		return vk::Format::eBc3UnormBlock;
	case EFormat::BC3SrgbBlock:
		return vk::Format::eBc3SrgbBlock;
	case EFormat::BC4UnormBlock:
		return vk::Format::eBc4UnormBlock;
	case EFormat::BC4SnormBlock:
		return vk::Format::eBc4SnormBlock;
	case EFormat::BC5UnormBlock:
		return vk::Format::eBc5UnormBlock;
	case EFormat::BC5SnormBlock:
		return vk::Format::eBc5SnormBlock;
	case EFormat::BC6HUfloatBlock:
		return vk::Format::eBc6HUfloatBlock;
	case EFormat::BC6HSfloatBlock:
		return vk::Format::eBc6HSfloatBlock;
	case EFormat::BC7UnormBlock:
		return vk::Format::eBc7UnormBlock;
	case EFormat::BC7SrgbBlock:
		return vk::Format::eBc7SrgbBlock;

	default:
		return vk::Format::eUndefined;
	}
}

WIND_INLINE auto ToVk(EVertexFormat Format) WIND_NOEXCEPT -> vk::Format
{
	switch (Format)
	{
	case EVertexFormat::Float:
		return vk::Format::eR32Sfloat;
	case EVertexFormat::Float2:
		return vk::Format::eR32G32Sfloat;
	case EVertexFormat::Float3:
		return vk::Format::eR32G32B32Sfloat;
	case EVertexFormat::Float4:
		return vk::Format::eR32G32B32A32Sfloat;

	case EVertexFormat::Int:
		return vk::Format::eR32Sint;
	case EVertexFormat::Int2:
		return vk::Format::eR32G32Sint;
	case EVertexFormat::Int3:
		return vk::Format::eR32G32B32Sint;
	case EVertexFormat::Int4:
		return vk::Format::eR32G32B32A32Sint;

	case EVertexFormat::UInt:
		return vk::Format::eR32Uint;
	case EVertexFormat::UInt2:
		return vk::Format::eR32G32Uint;
	case EVertexFormat::UInt3:
		return vk::Format::eR32G32B32Uint;
	case EVertexFormat::UInt4:
		return vk::Format::eR32G32B32A32Uint;

	default:
		return vk::Format::eR32Sfloat;
	}
}

WIND_INLINE auto ToVk(EVertexInputRate InputRate) WIND_NOEXCEPT -> vk::VertexInputRate
{
	switch (InputRate)
	{
	case EVertexInputRate::Vertex:
		return vk::VertexInputRate::eVertex;
	case EVertexInputRate::Instance:
		return vk::VertexInputRate::eInstance;
	default:
		return vk::VertexInputRate::eVertex;
	}
}

WIND_INLINE auto ToVk(const FVertexAttribute &Attribute) WIND_NOEXCEPT -> vk::VertexInputAttributeDescription
{
	return vk::VertexInputAttributeDescription{Attribute.Location, Attribute.Binding, ToVk(Attribute.Format),
	                                           Attribute.Offset};
}

WIND_INLINE auto ToVk(std::span<const FVertexAttribute> Attributes) WIND_NOEXCEPT
    -> std::vector<vk::VertexInputAttributeDescription>
{
	std::vector<vk::VertexInputAttributeDescription> VertexAttributes;
	VertexAttributes.reserve(Attributes.size());

	for (const auto &Attribute : Attributes)
	{
		VertexAttributes.emplace_back(Attribute.Location, Attribute.Binding, ToVk(Attribute.Format), Attribute.Offset);
	}

	return VertexAttributes;
}

WIND_INLINE auto ToVk(const FVertexBinding &Binding) WIND_NOEXCEPT -> vk::VertexInputBindingDescription
{
	return vk::VertexInputBindingDescription{Binding.Binding, Binding.Stride, ToVk(Binding.InputRate)};
}

WIND_INLINE auto ToVk(std::span<const FVertexBinding> Bindings) WIND_NOEXCEPT
    -> std::vector<vk::VertexInputBindingDescription>
{
	std::vector<vk::VertexInputBindingDescription> VertexBindings;
	VertexBindings.reserve(Bindings.size());

	for (const auto &Binding : Bindings)
	{
		VertexBindings.emplace_back(Binding.Binding, Binding.Stride, ToVk(Binding.InputRate));
	}

	return VertexBindings;
}

WIND_INLINE auto ToVk(const FVertexInputState &VertexInput) WIND_NOEXCEPT
    -> std::pair<std::vector<vk::VertexInputAttributeDescription>, std::vector<vk::VertexInputBindingDescription>>
{
	std::vector<vk::VertexInputAttributeDescription> Attributes;
	std::vector<vk::VertexInputBindingDescription> Bindings;

	Attributes.reserve(VertexInput.Attributes.size());
	Bindings.reserve(VertexInput.Bindings.size());

	for (const auto &Attr : VertexInput.Attributes)
		Attributes.push_back(ToVk(Attr));

	for (const auto &Bind : VertexInput.Bindings)
		Bindings.push_back(ToVk(Bind));

	return {std::move(Attributes), std::move(Bindings)};
}

WIND_INLINE auto ToVk(ECompareOp CompareOp) WIND_NOEXCEPT -> vk::CompareOp
{
	switch (CompareOp)
	{
	case ECompareOp::Less:
		return vk::CompareOp::eLess;
	case ECompareOp::Always:
		return vk::CompareOp::eAlways;
	case ECompareOp::Never:
		return vk::CompareOp::eNever;
	case ECompareOp::Greater:
		return vk::CompareOp::eGreater;
	default:
		return vk::CompareOp::eLess;
	}
}

WIND_INLINE auto ToVk(EShaderStage Stage) WIND_NOEXCEPT -> vk::ShaderStageFlagBits
{
	switch (Stage)
	{
	case EShaderStage::Vertex:
		return vk::ShaderStageFlagBits::eVertex;

	case EShaderStage::Fragment:
		return vk::ShaderStageFlagBits::eFragment;

	case EShaderStage::Compute:
		return vk::ShaderStageFlagBits::eCompute;
	}

	std::unreachable();
}

WIND_INLINE auto ToVkShaderStageFlags(EShaderStage Stages) WIND_NOEXCEPT -> vk::ShaderStageFlags
{
	vk::ShaderStageFlags Result{};

	if ((Stages & EShaderStage::Vertex) != EShaderStage{})
		Result |= vk::ShaderStageFlagBits::eVertex;

	if ((Stages & EShaderStage::Fragment) != EShaderStage{})
		Result |= vk::ShaderStageFlagBits::eFragment;

	if ((Stages & EShaderStage::Compute) != EShaderStage{})
		Result |= vk::ShaderStageFlagBits::eCompute;

	return Result;
}

WIND_INLINE auto ToVk(const FRasterizationState &Rasterization) WIND_NOEXCEPT
    -> vk::PipelineRasterizationStateCreateInfo
{
	vk::PipelineRasterizationStateCreateInfo Info = {};
	Info.depthClampEnable = static_cast<vk::Bool32>(Rasterization.DepthClamp);
	Info.rasterizerDiscardEnable = static_cast<vk::Bool32>(Rasterization.Discard);
	Info.polygonMode = ToVk(Rasterization.PolygonMode);
	Info.cullMode = ToVk(Rasterization.CullMode);
	Info.frontFace = ToVk(Rasterization.FrontFace);
	Info.lineWidth = 1.0F;

	return Info;
}

WIND_INLINE auto ToVk(const FInputAssemblyState &InputAssembly) WIND_NOEXCEPT
    -> vk::PipelineInputAssemblyStateCreateInfo
{
	vk::PipelineInputAssemblyStateCreateInfo CreateInfo{};
	CreateInfo.topology = ToVk(InputAssembly.Topology);
	CreateInfo.primitiveRestartEnable = vk::False;

	return CreateInfo;
}

WIND_INLINE auto ToVk(const FDepthStencilState &DepthStencil) WIND_NOEXCEPT -> vk::PipelineDepthStencilStateCreateInfo
{
	vk::PipelineDepthStencilStateCreateInfo Info{};

	Info.depthTestEnable = static_cast<vk::Bool32>(DepthStencil.DepthTest);
	Info.depthWriteEnable = static_cast<vk::Bool32>(DepthStencil.DepthWrite);
	Info.depthCompareOp = ToVk(DepthStencil.DepthCompare);
	Info.stencilTestEnable = static_cast<vk::Bool32>(DepthStencil.StencilTest);

	// Default stencil ops if not configured
	Info.front = vk::StencilOpState{vk::StencilOp::eKeep,
	                                vk::StencilOp::eKeep,
	                                vk::StencilOp::eKeep,
	                                vk::CompareOp::eAlways,
	                                0xFFFFFFFF,
	                                0xFFFFFFFF,
	                                0};

	Info.back = Info.front;

	return Info;
}

WIND_INLINE auto ToVk(EBlendFactor BlendFactor) WIND_NOEXCEPT -> vk::BlendFactor
{
	switch (BlendFactor)
	{
	case EBlendFactor::Zero:
		return vk::BlendFactor::eZero;
	case EBlendFactor::One:
		return vk::BlendFactor::eOne;
	case EBlendFactor::SrcColor:
		return vk::BlendFactor::eSrcColor;
	case EBlendFactor::OneMinusSrcColor:
		return vk::BlendFactor::eOneMinusSrcColor;
	case EBlendFactor::DstColor:
		return vk::BlendFactor::eDstColor;
	case EBlendFactor::OneMinusDstColor:
		return vk::BlendFactor::eOneMinusDstColor;
	case EBlendFactor::SrcAlpha:
		return vk::BlendFactor::eSrcAlpha;
	case EBlendFactor::OneMinusSrcAlpha:
		return vk::BlendFactor::eOneMinusSrcAlpha;
	case EBlendFactor::DstAlpha:
		return vk::BlendFactor::eDstAlpha;
	case EBlendFactor::OneMinusDstAlpha:
		return vk::BlendFactor::eOneMinusDstAlpha;
	case EBlendFactor::ConstantColor:
		return vk::BlendFactor::eConstantColor;
	case EBlendFactor::OneMinusConstantColor:
		return vk::BlendFactor::eOneMinusConstantColor;
	case EBlendFactor::ConstantAlpha:
		return vk::BlendFactor::eConstantAlpha;
	case EBlendFactor::OneMinusConstantAlpha:
		return vk::BlendFactor::eOneMinusConstantAlpha;
	case EBlendFactor::SrcAlphaSaturate:
		return vk::BlendFactor::eSrcAlphaSaturate;
	case EBlendFactor::Src1Color:
		return vk::BlendFactor::eSrc1Color;
	case EBlendFactor::OneMinusSrc1Color:
		return vk::BlendFactor::eOneMinusSrc1Color;
	case EBlendFactor::Src1Alpha:
		return vk::BlendFactor::eSrc1Alpha;
	case EBlendFactor::OneMinusSrc1Alpha:
		return vk::BlendFactor::eOneMinusSrc1Alpha;
	default:
		return vk::BlendFactor::eZero;
	}
}

WIND_INLINE auto ToVk(EBlendOp BlendOp) WIND_NOEXCEPT -> vk::BlendOp
{
	switch (BlendOp)
	{
	case EBlendOp::Add:
		return vk::BlendOp::eAdd;
	case EBlendOp::Subtract:
		return vk::BlendOp::eSubtract;
	case EBlendOp::ReverseSubtract:
		return vk::BlendOp::eReverseSubtract;
	case EBlendOp::Min:
		return vk::BlendOp::eMin;
	case EBlendOp::Max:
		return vk::BlendOp::eMax;
	default:
		return vk::BlendOp::eAdd;
	}
}

WIND_INLINE auto ToVk(const FPushConstantRange &PushConstant) WIND_NOEXCEPT -> vk::PushConstantRange
{
	vk::PushConstantRange VkPushConstant{};
	VkPushConstant.offset = PushConstant.Offset;
	VkPushConstant.size = PushConstant.Size;
	VkPushConstant.stageFlags = ToVkShaderStageFlags(PushConstant.StageFlags);
	return VkPushConstant;
}

WIND_INLINE auto ToVk(const FColorBlendState &ColorBlend) WIND_NOEXCEPT -> vk::PipelineColorBlendAttachmentState
{
	vk::PipelineColorBlendAttachmentState State{};

	if (!ColorBlend.Enabled)
	{
		State.blendEnable = vk::False;
		State.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                       vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		return State;
	}

	State.blendEnable = vk::True;
	State.srcColorBlendFactor = ToVk(ColorBlend.SrcColor);
	State.dstColorBlendFactor = ToVk(ColorBlend.DstColor);
	State.colorBlendOp = ToVk(ColorBlend.ColorOp);
	State.srcAlphaBlendFactor = ToVk(ColorBlend.SrcAlpha);
	State.dstAlphaBlendFactor = ToVk(ColorBlend.DstAlpha);
	State.alphaBlendOp = ToVk(ColorBlend.AlphaOp);
	State.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	                       vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	return State;
}

// Helper function to convert entire GraphicsConfig
struct FPipelineCreateInfo
{
	std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages;
	std::vector<vk::VertexInputAttributeDescription> Attributes;
	std::vector<vk::VertexInputBindingDescription> Bindings;
	vk::PipelineInputAssemblyStateCreateInfo InputAssembly;
	vk::PipelineRasterizationStateCreateInfo Rasterization;
	vk::PipelineDepthStencilStateCreateInfo DepthStencil;
	vk::PipelineColorBlendAttachmentState ColorBlendAttachment;
	vk::PipelineViewportStateCreateInfo ViewportState;
	vk::PipelineMultisampleStateCreateInfo Multisample;
};

WIND_INLINE auto ToVk(const FGraphicsConfig &Config) WIND_NOEXCEPT -> FPipelineCreateInfo
{
	FPipelineCreateInfo Result{};

	// Convert shader stages
	std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages;
	ShaderStages.reserve(Config.Shader.size());

	for (const auto &ShaderInfo : Config.Shader)
	{
		vk::PipelineShaderStageCreateInfo Info{};
		Info.stage = ToVk(ShaderInfo.Stage);
		Info.module = *ShaderInfo.Module;
		Info.pName = ShaderInfo.EntryPoint.data();

		ShaderStages.push_back(Info);
	}

	Result.Bindings = ToVk(Config.VertexInputState.Bindings);
	Result.Attributes = ToVk(Config.VertexInputState.Attributes);

	// Convert other states
	auto InputAssembly = ToVk(Config.InputAssembly);
	auto Rasterization = ToVk(Config.Rasterization);
	auto DepthStencil = ToVk(Config.DepthStencil);
	Result.ColorBlendAttachment = ToVk(Config.ColorBlend);

	// Viewport state (default)
	vk::PipelineViewportStateCreateInfo ViewportState{};
	ViewportState.viewportCount = 1;
	ViewportState.scissorCount = 1;

	// Multisample state (default)
	vk::PipelineMultisampleStateCreateInfo Multisample{};
	Multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

	Result.ShaderStages = std::move(ShaderStages);
	Result.InputAssembly = InputAssembly;
	Result.Rasterization = Rasterization;
	Result.DepthStencil = DepthStencil;
	Result.ViewportState = ViewportState;
	Result.Multisample = Multisample;

	return Result;
}
