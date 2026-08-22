#pragma once

#include "config.hpp"
#include "resources/resource_manager.hpp"
#include "spdlog/spdlog.h"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"
#include "vulkan/frame/frame_context.hpp"

namespace wind::vulkan {

class Renderer
{
public:
  Renderer(const auto&)                    = delete;
  auto operator=(const auto&) -> Renderer& = delete;

  WIND_NODISCARD static auto create(Context context) WIND_NOEXCEPT -> WindResult<Renderer>;

  auto test() -> WindResult<void>
  {
    auto vert_shader = WIND_TRY(m_resource_manager.load_shader(m_context.device_ctx.device, "assets/shaders/vert.spv"));

    auto frag_shader = WIND_TRY(m_resource_manager.load_shader(m_context.device_ctx.device, "assets/shaders/frag.spv"));

    spdlog::info("got shader handle: {}", vert_shader.index);
    spdlog::info("got shader handle: {}", frag_shader.index);

    auto* shader_data = WIND_TRY(m_resource_manager.get_shader(vert_shader));

    spdlog::info("{}", (void*)shader_data);

    return {};
  }

private:
  Renderer(Context context, std::vector<FrameContext> frame_context)
      : m_context{std::move(context)}
      , m_frame_context(std::move(frame_context)) {};

  Context                   m_context;
  std::vector<FrameContext> m_frame_context;
  ResourceManager           m_resource_manager;
};

}  // namespace wind::vulkan
