#pragma once

#include "platform/window.hpp"
#include "utils/expected_util.hpp"
#include "vulkan/core/context.hpp"

namespace wind::app {
struct Context
{
  platform::Window window;
  vulkan::Context  ctx;
};

[[nodiscard]] auto init(platform::WindowConfiguration win_cfg) noexcept -> WindResult<Context>;

auto run() noexcept -> void;

};  // namespace wind::app
