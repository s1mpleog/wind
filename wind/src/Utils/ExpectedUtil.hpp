#pragma once

#include "Error.hpp"

#include <expected>

template <typename T>
using TWindResult = std::expected<T, WindError>;

template <typename T>
using WindVkResult = std::expected<T, vk::Result>;

inline auto to_wind_error(vk::Result result, ErrorCode code = ErrorCode::InternalError) -> WindError
{
	return WindError::vulkan(code, result);
}

inline auto to_wind_error(WindError error) -> WindError
{
	return error;
}

#define WIND_ENSURE_NOT_EMPTY(range, error)                                                                            \
	{                                                                                                                  \
		if (std::ranges::empty(range))                                                                                 \
			WIND_ERR(error);                                                                                           \
	}

#define WIND_TRY(expr, ...)                                                                                            \
	({                                                                                                                 \
		auto wind_try_result = (expr);                                                                                 \
		if (!wind_try_result)                                                                                          \
			return std::unexpected(to_wind_error(wind_try_result.error() __VA_OPT__(, __VA_ARGS__)));                  \
		std::move(wind_try_result).value();                                                                            \
	})

#define WIND_TRY_VOID(expr)                                                                                            \
	({                                                                                                                 \
		auto wind_try_result = (expr);                                                                                 \
		if (!wind_try_result)                                                                                          \
			return std::unexpected(wind_try_result.error());                                                           \
	})

#define WIND_ERR(expr) return std::unexpected(expr)
