#pragma once

#include <expected>
#include "error.hpp"

template <typename T>
using WindResult = std::expected<T, WindError>;

#define WIND_TRY(expr)                                                                                                 \
  ({                                                                                                                   \
    auto r = (expr);                                                                                                   \
    if(!r)                                                                                                             \
      return std::unexpected(r.error());                                                                               \
    r.value();                                                                                                         \
  })

#define WIND_TRY_VOID(expr)                                                                                            \
  ({                                                                                                                   \
    auto r = (expr);                                                                                                   \
    if(!r)                                                                                                             \
      return std::unexpected(r.error());                                                                               \
  })
