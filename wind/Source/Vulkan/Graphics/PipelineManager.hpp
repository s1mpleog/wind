#pragma once

#include "Config.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/Pipeline.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"

#include <vector>

using TPipelineHandle = uint32;

class FUPipelineManager
{
	// TODO: cache it
  public:
	FUPipelineManager() = default;

	FUPipelineManager(const FUPipelineManager &) = delete;
	auto operator=(const FUPipelineManager &) -> FUPipelineManager & = delete;

	FUPipelineManager(FUPipelineManager &&) = default;
	auto operator=(FUPipelineManager &&) -> FUPipelineManager & = default;

	WIND_NODISCARD auto WIND_INLINE Create(FGraphicsConfig Config, const vk::raii::Device &Device) WIND_NOEXCEPT
	    -> TWindResult<TPipelineHandle>
	{
		TPipelineHandle Handle = MPipelines.size();
		MPipelines.emplace_back(WIND_TRY(CreatePipeline(Device, std::move(Config))));
		return Handle;
	}

	auto Get(TPipelineHandle Handle) -> TWindResult<FGraphicsPipeline *>
	{
		if (Handle > MPipelines.size())
			WIND_ERR(WindError::internal());

		return &MPipelines[Handle];
	};

	auto GetUnchecked(TPipelineHandle Handle) -> FGraphicsPipeline *
	{
		return &MPipelines[Handle];
	}

  private:
	std::vector<FGraphicsPipeline> MPipelines;
};
