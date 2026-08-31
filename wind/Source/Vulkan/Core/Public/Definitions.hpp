#pragma once

#include "Config.hpp"

enum class EGpuVendorId : uint32
{
	Unknown = 0xffffffff,
	NotQueried = 0,

	Amd = 0x1002,
	Nvidia = 0x10DE,
	Microsoft = 0x1414,

};

WIND_INLINE EGpuVendorId ConvertToGpuVendorId(uint32 VendorId)
{
	switch ((EGpuVendorId)VendorId)
	{
	case EGpuVendorId::NotQueried:
		return EGpuVendorId::NotQueried;

	case EGpuVendorId::Amd:
	case EGpuVendorId::Nvidia:
	case EGpuVendorId::Microsoft:
		return (EGpuVendorId)VendorId;

	case EGpuVendorId::Unknown:
	default:
		return EGpuVendorId::Unknown;
	}
}
