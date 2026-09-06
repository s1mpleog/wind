#pragma once

#include "Error.hpp"

#include <format>
#include <spdlog/spdlog.h>

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

#ifdef WIND_LOG_ENABLE

#define WIND_LOG(level, fmt, ...) spdlog::level(fmt, ##__VA_ARGS__)

#else

#define WIND_LOG(level, fmt, ...)                                                                                      \
	do                                                                                                                 \
	{                                                                                                                  \
	} while (0)

#endif
