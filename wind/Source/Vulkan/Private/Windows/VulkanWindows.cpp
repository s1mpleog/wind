#include "VulkanWindows.hpp"

void FVulkanPlatformWindows::GetInstanceExtensions(std::vector<const char *> &OutExtensions)
{
	OutExtensions.emplace_back("VK_KHR_win32_surface");
}
