#include "Input/InputManager.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

namespace wind::input {

InputManger::InputManger()
    : m_key_states(SDL_GetKeyboardState(nullptr))
{
}

auto InputManger::process_event(SDL_Event& event) WIND_NOEXCEPT -> void
{
  switch(event.type)
  {
    case SDL_EVENT_MOUSE_MOTION:
      m_mouse_x = event.motion.x;
      m_mouse_y = event.motion.y;
      m_mouse_x_rel += event.motion.xrel;
      m_mouse_y_rel += event.motion.yrel;
      break;

    default:
      break;
  }
}

WIND_NODISCARD auto InputManger::get_mouse_position() const WIND_NOEXCEPT -> MousePosition
{
  return MousePosition{
      .x     = m_mouse_x,
      .y     = m_mouse_y,
      .y_rel = m_mouse_y_rel,
      .x_rel = m_mouse_x_rel,
  };
}

};  // namespace wind::input
