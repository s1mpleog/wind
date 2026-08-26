#include "input_manager.hpp"
#include "SDL3/SDL_keyboard.h"

namespace wind::input {

InputManger::InputManger()
    : m_key_states(SDL_GetKeyboardState(nullptr))
{
}

};  // namespace wind::input
