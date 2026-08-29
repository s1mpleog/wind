#pragma once

struct FOptionalVulkanDeviceExtensions
{
	union
	{
		struct
		{
			uint64 HasMemoryBudget : 1;
			uint64 HasKHRMaintenance4 : 1;
			uint64 HasKHRPushDescriptors : 1;
			uint64 HasEXTLoadStoreOpNone : 1;
			uint64 HasEXTHostImageCopy : 1;
		};
		uint64 Packed;
	};

	FOptionalVulkanDeviceExtensions() : Packed(0) {};
};
