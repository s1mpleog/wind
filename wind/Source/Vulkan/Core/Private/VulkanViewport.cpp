#include "Vulkan/Core/Private/VulkanViewport.hpp"

FVulkanViewport::FVulkanViewport(FVulkanDevice &InDevice, void *InWindowHandle, uint32 InSizeX, uint32 InSizeY)
    : Device(InDevice), WindowHandle(InWindowHandle), SizeX(InSizeX), SizeY(InSizeY), SwapChain(nullptr) {};

void FVulkanViewport::CreateSwapchain()
{
}
