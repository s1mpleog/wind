#include "Resources/DescriptorManager.hpp"

#include "Config.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "spdlog/spdlog.h"

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

WIND_NODISCARD auto UDescriptorManager::Create(const vk::raii::Device &Device,
                                               vk::DescriptorType DescriptorType) WIND_NOEXCEPT
    -> WindResult<UDescriptorManager>
{
	// todo: don't use luck number
	vk::DescriptorPoolSize PoolSize{};
	PoolSize.descriptorCount = 1024;
	PoolSize.type = DescriptorType;

	vk::DescriptorPoolCreateInfo DsPoolCreateInfo{};
	DsPoolCreateInfo.flags =
	    vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	DsPoolCreateInfo.pPoolSizes = &PoolSize;
	DsPoolCreateInfo.poolSizeCount = 1;
	DsPoolCreateInfo.maxSets = 1024;

	auto Pool = WIND_TRY(Device.createDescriptorPool(DsPoolCreateInfo));

#ifdef WIND_LOG_ENABLE
	spdlog::info("created {} descriptor pools with layout: {}", PoolSize.descriptorCount,
	             vk::to_string(DescriptorType));
#endif

	return UDescriptorManager{std::move(Pool)};
}

WIND_NODISCARD auto UDescriptorManager::CreateSet(const vk::raii::Device &Device) WIND_NOEXCEPT -> WindResult<void>
{
	WIND_ASSERT(MPool != nullptr && "Descriptor pool is null while trying to allocate");
	WIND_ASSERT(MLayout != VK_NULL_HANDLE && "layout is null oops created layout first");

	vk::DescriptorSetAllocateInfo AllocInfo{};
	AllocInfo.descriptorPool = MPool;
	AllocInfo.descriptorSetCount = 1;
	AllocInfo.pSetLayouts = &*MLayout;

	auto Result = WIND_TRY(Device.allocateDescriptorSets(AllocInfo));

#ifdef WIND_LOG_ENABLE
	spdlog::info("descriptor set created successfully");
#endif

	MSet = std::move(Result.front());

	return {};
}

WIND_NODISCARD auto
UDescriptorManager::CreateLayout(const vk::raii::Device &Device,
                                 std::span<const vk::DescriptorSetLayoutBinding> Bindings) WIND_NOEXCEPT
    -> WindResult<void>
{
	vk::DescriptorSetLayoutCreateInfo LayoutCreateInfo{};
	LayoutCreateInfo.bindingCount = static_cast<u32>(Bindings.size());
	LayoutCreateInfo.pBindings = Bindings.data();

	MLayout = WIND_TRY(Device.createDescriptorSetLayout(LayoutCreateInfo));

	return {};
}

WIND_NODISCARD auto UDescriptorManager::GetPool() const -> const vk::raii::DescriptorPool *
{
	WIND_ASSERT(MPool != nullptr && "pool is null");
	return &MPool;
}

WIND_NODISCARD auto UDescriptorManager::GetSet() const -> const vk::raii::DescriptorSet *
{
	WIND_ASSERT(MSet != nullptr && "set is nullptr");
	return &MSet;
}

WIND_NODISCARD auto UDescriptorManager::GetLayout() const -> const vk::raii::DescriptorSetLayout *
{
	WIND_ASSERT(MLayout != nullptr && "layout is nullptr");
	return &MLayout;
}
