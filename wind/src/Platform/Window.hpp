#pragma once

/* responsible to create platfrom agnostic window and expose functions for vulkan */

#include "Config.hpp"
#include "SDL3/SDL_video.h"
#include "Types.hpp"
#include "Utils/ExpectedUtil.hpp"

#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

struct FWindowConfiguration
{
	std::string Name;
	u16 Width{};
	u16 Height{};
};

inline auto MakeWindowConfig(const u16 Width, const u16 Height, std::string Name) WIND_NOEXCEPT -> FWindowConfiguration
{
	return FWindowConfiguration{.Name = std::move(Name), .Width = Width, .Height = Height};
}

class UWindow
{
  public:
	explicit UWindow(FWindowConfiguration Cfg) : MConfig{std::move(Cfg)} {};

	UWindow(const UWindow &) = delete;
	auto operator=(const UWindow &) -> UWindow = delete;

	UWindow(UWindow &&Other) WIND_NOEXCEPT : MConfig{std::move(Other.MConfig)},
	                                         MHandle{std::exchange(Other.MHandle, nullptr)} {};

	auto operator=(UWindow &&Other) WIND_NOEXCEPT->UWindow &
	{
		if (this != &Other)
		{
			if (MHandle != nullptr)
				SDL_DestroyWindow(MHandle);

			MHandle = Other.MHandle;
			MConfig = std::move(Other.MConfig);

			Other.MConfig = {};
			Other.MHandle = nullptr;
		}

		return *this;
	};

	WIND_NODISCARD auto Create() WIND_NOEXCEPT -> WindResult<void>;
	WIND_NODISCARD auto Extensions() const WIND_NOEXCEPT -> WindResult<std::vector<const char *>>;
	WIND_NODISCARD auto CreateSurface(const vk::Instance &Instance) const WIND_NOEXCEPT -> WindResult<VkSurfaceKHR>;
	[[nodiscard]] auto GetConfig() const WIND_NOEXCEPT -> const FWindowConfiguration &;

	WIND_NODISCARD auto DrawableSize() const WIND_NOEXCEPT -> std::pair<u32, u32>
	{
		return {MConfig.Width, MConfig.Height};
	}

	WIND_NODISCARD auto Handle() WIND_NOEXCEPT -> SDL_Window *
	{
		return MHandle;
	}

	~UWindow()
	{
		if (MHandle != nullptr)
		{
			SDL_DestroyWindow(MHandle);
#ifdef WIND_LOG_ENABLE
			spdlog::info("window handler destroyed");
#endif
		}
	}

  private:
	FWindowConfiguration MConfig{};
	SDL_Window *MHandle{nullptr};
};
