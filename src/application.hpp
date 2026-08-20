#pragma once

#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/configuration.hpp"
#include "vulkan/core/context.hpp"

namespace wind::app {
struct Context
{
  platform::Window window;
  vulkan::Context  ctx;
};

WIND_NODISCARD auto init(platform::WindowConfiguration win_cfg, vulkan::Configuration vulkan_cfg) WIND_NOEXCEPT -> WindResult<Context>;

auto run() WIND_NOEXCEPT -> void;

};  // namespace wind::app
