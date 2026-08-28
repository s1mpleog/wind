#include "Vulkan/Core/Context.hpp"

#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Core/Instance.hpp"
#include "Vulkan/Core/ValidationLayer.hpp"

auto CreateContext(const FUWindow &Window, const FConfiguration &Cfg) WIND_NOEXCEPT -> TWindResult<FVulkanContext>
{
	FVulkanContext Ctx{};

	auto PlatformExtensions = WIND_TRY(Window.Extensions());
	Ctx.Instance = WIND_TRY(Create(Cfg, Ctx.RaiiCtx, std::move(PlatformExtensions)));

#ifdef WIND_VULKAN_VALIDATION
#ifdef WIND_LOG_ENABLE
	spdlog::info("WIND_VULKAN_VALIDATION enabled, setting up debug messenger");
#endif
	Ctx.Messenger = WIND_TRY(CreateDebugUtils(Cfg, Ctx.Instance));
#endif

	auto *SurfaceRaw = WIND_TRY(Window.CreateSurface(Ctx.Instance));
	Ctx.Surface = vk::raii::SurfaceKHR(Ctx.Instance, SurfaceRaw);

	Ctx.GpuDevice = WIND_TRY(DeviceCreate(Cfg, Ctx.Instance, Ctx.Surface));

	return Ctx;
}
