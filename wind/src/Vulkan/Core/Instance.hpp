#pragma once

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"
#include <vulkan/vulkan_core.h>

WIND_NODISCARD auto create(const Configuration& cfg, const vk::raii::Context& ctx, std::vector<const char*> extensions) WIND_NOEXCEPT
    -> WindResult<vk::raii::Instance>;
