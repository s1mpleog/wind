#pragma once

#include <cstdint>
#include <string_view>

void Fatal(std::string_view Expression, std::string_view Error, const char *File, uint32_t Line) noexcept;
