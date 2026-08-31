#pragma once

#include "Core/Internal/CrashHandler.hpp"

#include <spdlog/spdlog.h>
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

#define CHECK_1(cond)                                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(cond))                                                                                                   \
		{                                                                                                              \
			Fatal(#cond, "check failed", __FILE__, __LINE__);                                                          \
		}                                                                                                              \
	} while (0)

#define CHECK_2(cond, msg)                                                                                             \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(cond))                                                                                                   \
		{                                                                                                              \
			Fatal(#cond, msg, __FILE__, __LINE__);                                                                     \
		}                                                                                                              \
	} while (0)

#define CHECK_GET_MACRO(_1, _2, NAME, ...) NAME
#define CHECK(...) CHECK_GET_MACRO(__VA_ARGS__, CHECK_2, CHECK_1)(__VA_ARGS__)
#define FATAL(fmt, ...) Fatal("", std::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__)

// TODO: this does not blongs here move when we have logging subsystem
#ifdef WIND_LOG_ENABLE
#define WIND_LOG(level, fmt, ...) spdlog::level(fmt, ##__VA_ARGS__)
#else
#define WIND_LOG(level, fmt, ...)                                                                                      \
	do                                                                                                                 \
	{                                                                                                                  \
	} while (0)
#endif
