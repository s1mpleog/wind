#pragma once

#include "Types.hpp"

#include <Config.hpp>
#include <string_view>

void Fatal(std::string_view Expression, std::string_view Error, const char *File, uint32 Line) WIND_NOEXCEPT;
