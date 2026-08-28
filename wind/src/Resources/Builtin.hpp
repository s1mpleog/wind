#pragma once

#include "Resources/ResourceManager.hpp"
#include "Vulkan/Graphics/PipelineManager.hpp"

#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct FModelAssets
{
	// descriptor set and push constant exists for model
	TModelHandle Model{};
};

struct FBufferAssets
{
	TBufferHandle VertexHandle{};
	TBufferHandle IndexHandle{};

	u32 IndexCount{};
	u32 VertexCount{};
};

struct FBuiltAssets
{
	TModelHandle Models{};
	TPipelineHandle Pipelines{};
	bool HasDescriptorSets{false};
	bool IsModel{true};
	FBufferAssets BufferAssets{};
};

WIND_NODISCARD auto Build(UResourceManager *ResourceManager, UPipelineManager *PipelineManager,
                          const vk::raii::Device &Device) WIND_NOEXCEPT -> WindResult<std::vector<FBuiltAssets>>;
