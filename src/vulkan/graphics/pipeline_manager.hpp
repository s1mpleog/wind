#pragma once

#include "config.hpp"
#include "error.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/graphics/pipeline.hpp"
#include "vulkan/graphics/pipeline_config.hpp"
#include <vector>

namespace wind::vulkan::graphics {

using PipelineId = u32;

class PipelineManager
{
  //TODO: cache it
public:
  PipelineManager() = default;

  PipelineManager(const PipelineManager&)                    = delete;
  auto operator=(const PipelineManager&) -> PipelineManager& = delete;

  PipelineManager(PipelineManager&&)                    = default;
  auto operator=(PipelineManager&&) -> PipelineManager& = default;

  WIND_NODISCARD auto WIND_INLINE create(GraphicsConfig config, const vk::raii::Device& device) WIND_NOEXCEPT -> WindResult<PipelineId>
  {
    PipelineId id = m_pipelines.size();
    m_pipelines.emplace_back(WIND_TRY(graphics::create(device, std::move(config))));
    return id;
  }

  auto get(PipelineId id) -> WindResult<GraphicsPipeline*>
  {
    if(id > m_pipelines.size())
      WIND_ERR(WindError::internal());

    return &m_pipelines[id];
  };

private:
  std::vector<GraphicsPipeline> m_pipelines;
};


}  // namespace wind::vulkan::graphics
