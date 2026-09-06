#pragma once

// #include "ApplicationCore/Public/GenericPlatform/GenericWindow.hpp"
#include "GenericPlatform/GenericWindow.hpp"

#include <SDL3/SDL.h>
#include <utility>

class FLinuxWindow : public FGenericWindow
{
  public:
	FLinuxWindow(FGenericWindowParams InWindowParams);
	~FLinuxWindow();

	FLinuxWindow(const FLinuxWindow &) = delete;
	FLinuxWindow &operator=(const FLinuxWindow &) = delete;

	FLinuxWindow(FLinuxWindow &&Other) noexcept
	    : HWnd{std::exchange(Other.HWnd, nullptr)}, bIsSdlInit(std::exchange(Other.bIsSdlInit, false)),
	      WindowParams{std::move(Other.WindowParams)} {};

	FLinuxWindow &operator=(FLinuxWindow &&Other) noexcept
	{
		if (this != &Other)
		{
			if (HWnd != nullptr)
				SDL_DestroyWindow(HWnd);

			HWnd = Other.HWnd;
			WindowParams = std::move(Other.WindowParams);
			bIsSdlInit = std::exchange(Other.bIsSdlInit, false);

			Other.WindowParams = {};
			Other.HWnd = nullptr;
		}

		return *this;
	};

	void Initialize();
	virtual void Destroy() override;

	virtual void *GetOSWindowHandle() override
	{
		return HWnd;
	}

	virtual void Show() override {};
	virtual void Hide() override {};

  private:
	SDL_Window *HWnd;
	bool bIsSdlInit = false;
	FGenericWindowParams WindowParams;
};
