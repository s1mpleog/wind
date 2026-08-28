#pragma once

#include "Config.hpp"
#include "Error.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Graphics/Pipeline.hpp"
#include "Vulkan/Graphics/PipelineConfig.hpp"

#include <vector>

using TPipelineHandle = u32;

class UPipelineManager
{
	// TODO: cache it
  public:
	UPipelineManager() = default;

	UPipelineManager(const UPipelineManager &) = delete;
	auto operator=(const UPipelineManager &) -> UPipelineManager & = delete;

	UPipelineManager(UPipelineManager &&) = default;
	auto operator=(UPipelineManager &&) -> UPipelineManager & = default;

	WIND_NODISCARD auto WIND_INLINE Create(FGraphicsConfig Config, const vk::raii::Device &Device) WIND_NOEXCEPT
	    -> WindResult<TPipelineHandle>
	{
		TPipelineHandle Handle = MPipelines.size();
		MPipelines.emplace_back(WIND_TRY(CreatePipeline(Device, std::move(Config))));
		return Handle;
	}

	auto Get(TPipelineHandle Handle) -> WindResult<FGraphicsPipeline *>
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
