#pragma once

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "Config.hpp"


struct MousePosition
{
  float x{};
  float y{};
  float y_rel{};
  float x_rel{};
};

class InputManger
{
public:
  InputManger();

  auto update() const -> void { SDL_PumpEvents(); }

  // TODO: replace SDL_Keycode with mine
  WIND_NODISCARD auto is_down(SDL_Keycode key) const WIND_NOEXCEPT -> bool { return m_key_states[key]; }

  auto process_event(SDL_Event& event) WIND_NOEXCEPT -> void;

  WIND_NODISCARD auto get_mouse_position() const WIND_NOEXCEPT -> MousePosition;

  auto begin_frame() WIND_NOEXCEPT -> void
  {
    m_mouse_x_rel = 0;
    m_mouse_y_rel = 0;
  }

private:
  const bool* m_key_states{};
  float       m_mouse_x{};
  float       m_mouse_y{};
  float       m_mouse_y_rel{};
  float       m_mouse_x_rel{};
};

