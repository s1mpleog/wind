#pragma once

#include <vector>

using FVulkanDeviceExtensionArray = std::vector<std::unique_ptr<class FVulkanDeviceExtension>>;
using FVulkanInstanceExtensionArray = std::vector<std::unique_ptr<class FVulkanInstanceExtension>>;
