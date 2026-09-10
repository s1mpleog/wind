#pragma once

#include "Config.hpp"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"

struct FMousePosition
{
	float X{};
	float Y{};
	float YRel{};
	float XRel{};
};

class FUInputManger
{
  public:
	FUInputManger();

	static auto Update() -> void
	{
		SDL_PumpEvents();
	}

	// TODO: replace SDL_Keycode with mine
	WIND_NODISCARD auto IsDown(SDL_Keycode Key) const WIND_NOEXCEPT -> bool
	{
		return MKeyStates[Key];
	}

	auto ProcessEvent(SDL_Event &Event) WIND_NOEXCEPT -> void;

	WIND_NODISCARD auto GetMousePosition() const WIND_NOEXCEPT -> FMousePosition;

	auto BeginFrame() WIND_NOEXCEPT -> void
	{
		MMouseXRel = 0;
		MMouseYRel = 0;
	}

  private:
	const bool *MKeyStates{};
	float MMouseX{};
	float MMouseY{};
	float MMouseYRel{};
	float MMouseXRel{};
};
