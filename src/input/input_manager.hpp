#pragma once

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "config.hpp"

namespace wind::input {
class InputManger
{
public:
  InputManger();

  auto update() const -> void { SDL_PumpEvents(); }

  // TODO: replace SDL_Keycode with mine
  WIND_NODISCARD auto is_down(SDL_Keycode key) const WIND_NOEXCEPT -> bool { return m_key_states[key]; }

private:
  const bool* m_key_states{};
};

}  // namespace wind::input
