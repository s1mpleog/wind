#include "Input/InputManager.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

UInputManger::UInputManger() : MKeyStates(SDL_GetKeyboardState(nullptr))
{
}

auto UInputManger::ProcessEvent(SDL_Event &Event) WIND_NOEXCEPT -> void
{
	switch (Event.type)
	{
	case SDL_EVENT_MOUSE_MOTION:
		MMouseX = Event.motion.x;
		MMouseY = Event.motion.y;
		MMouseXRel += Event.motion.xrel;
		MMouseYRel += Event.motion.yrel;
		break;

	default:
		break;
	}
}

WIND_NODISCARD auto UInputManger::GetMousePosition() const WIND_NOEXCEPT -> FMousePosition
{
	return FMousePosition{
	    .X = MMouseX,
	    .Y = MMouseY,
	    .YRel = MMouseYRel,
	    .XRel = MMouseXRel,
	};
}
