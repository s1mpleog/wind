#pragma once

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Configuration.hpp"

#include <vulkan/vulkan_core.h>

WIND_NODISCARD auto Create(const FConfiguration &Cfg, const vk::raii::Context &Ctx,
                           std::vector<const char *> Extensions) WIND_NOEXCEPT -> TWindResult<vk::raii::Instance>;
