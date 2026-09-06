#pragma once

#include "Check.hpp"

#include <vulkan/vulkan.hpp>

#define VERIFYVULKANRESULT(expr)                                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		vk::Result _res = static_cast<vk::Result>(expr);                                                               \
		if (_res != vk::Result::eSuccess)                                                                              \
		{                                                                                                              \
			Fatal(#expr, vk::to_string(_res), __FILE__, __LINE__);                                                     \
		}                                                                                                              \
	} while (0)

#define VERIFYVULKANRESULT_INIT(expr)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		vk::Result _res = static_cast<vk::Result>(expr);                                                               \
		if (_res == vk::Result::eErrorInitializationFailed)                                                            \
		{                                                                                                              \
			Fatal(#expr,                                                                                               \
			      "Initialization failed — Vulkan may not be properly set up on "                                      \
			      "this system. Try running vulkaninfo from the Vulkan SDK.",                                          \
			      __FILE__, __LINE__);                                                                                 \
		}                                                                                                              \
		else if (_res != vk::Result::eSuccess)                                                                         \
		{                                                                                                              \
			Fatal(#expr, vk::to_string(_res), __FILE__, __LINE__);                                                     \
		}                                                                                                              \
	} while (0)

#define VERIFYVULKANRESULT_UNWRAP(var, expr)                                                                           \
	auto _vk_exp_##var = (expr);                                                                                       \
	if (!_vk_exp_##var.has_value())                                                                                    \
	{                                                                                                                  \
		Fatal(#expr, vk::to_string(_vk_exp_##var.error()), __FILE__, __LINE__);                                        \
	}                                                                                                                  \
	auto &var = _vk_exp_##var.value()
