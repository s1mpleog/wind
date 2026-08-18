#pragma once

#include <expected>
#include "error.hpp"

template <typename T>
using WindResult = std::expected<T, WindError>;
