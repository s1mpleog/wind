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
	uint16 Width{};
	uint16 Height{};
};

inline auto MakeWindowConfig(const uint16 Width, const uint16 Height, std::string Name) WIND_NOEXCEPT -> FWindowConfiguration
{
	return FWindowConfiguration{.Name = std::move(Name), .Width = Width, .Height = Height};
}

class FUWindow
{
  public:
	explicit FUWindow(FWindowConfiguration Cfg) : MConfig{std::move(Cfg)} {};

	FUWindow(const FUWindow &) = delete;
	auto operator=(const FUWindow &) -> FUWindow = delete;

	FUWindow(FUWindow &&Other) WIND_NOEXCEPT : MConfig{std::move(Other.MConfig)},
	                                         MHandle{std::exchange(Other.MHandle, nullptr)} {};

	auto operator=(FUWindow &&Other) WIND_NOEXCEPT->FUWindow &
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

	WIND_NODISCARD auto Create() WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto Extensions() const WIND_NOEXCEPT -> TWindResult<std::vector<const char *>>;
	WIND_NODISCARD auto CreateSurface(const vk::Instance &Instance) const WIND_NOEXCEPT -> TWindResult<VkSurfaceKHR>;
	[[nodiscard]] auto GetConfig() const WIND_NOEXCEPT -> const FWindowConfiguration &;

	WIND_NODISCARD auto DrawableSize() const WIND_NOEXCEPT -> std::pair<uint32, uint32>
	{
		return {MConfig.Width, MConfig.Height};
	}

	WIND_NODISCARD auto Handle() WIND_NOEXCEPT -> SDL_Window *
	{
		return MHandle;
	}

	~FUWindow()
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
