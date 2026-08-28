#pragma once

#include "Config.hpp"
#include "./Error.hpp"
#include "utils/ExpectedUtil.hpp"
#include "vulkan/graphics/Pipeline.hpp"
#include "vulkan/graphics/PipelineConfig.hpp"
#include <vector>

namespace wind::vulkan::graphics {

using PipelineHandle = u32;

class PipelineManager
{
  //TODO: cache it
public:
  PipelineManager() = default;

  PipelineManager(const PipelineManager&)                    = delete;
  auto operator=(const PipelineManager&) -> PipelineManager& = delete;

  PipelineManager(PipelineManager&&)                    = default;
  auto operator=(PipelineManager&&) -> PipelineManager& = default;

  WIND_NODISCARD auto WIND_INLINE create(GraphicsConfig config, const vk::raii::Device& device) WIND_NOEXCEPT
      -> WindResult<PipelineHandle>
  {
    PipelineHandle handle = m_pipelines.size();
    m_pipelines.emplace_back(WIND_TRY(graphics::create(device, std::move(config))));
    return handle;
  }

  auto get(PipelineHandle handle) -> WindResult<GraphicsPipeline*>
  {
    if(handle > m_pipelines.size())
      WIND_ERR(WindError::internal());

    return &m_pipelines[handle];
  };

  auto get_unchecked(PipelineHandle handle) -> GraphicsPipeline* { return &m_pipelines[handle]; }

private:
  std::vector<GraphicsPipeline> m_pipelines;
};


}  // namespace wind::vulkan::graphics
