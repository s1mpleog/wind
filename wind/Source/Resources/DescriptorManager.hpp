#pragma once

#include "Config.hpp"
#include "Utils/ExpectedUtil.hpp"
#include "Vulkan/Memory/ResourceTypes.hpp"
#include "spdlog/spdlog.h"

#include <span>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

using TDescriptorHandle = uint32_t;

class FUDescriptorManager
{
  public:
	WIND_NODISCARD static auto Create(const vk::raii::Device &Device, vk::DescriptorType DescriptorType) WIND_NOEXCEPT
	    -> TWindResult<FUDescriptorManager>;
	WIND_NODISCARD auto CreateSet(const vk::raii::Device &Device) WIND_NOEXCEPT -> TWindResult<void>;
	WIND_NODISCARD auto CreateLayout(const vk::raii::Device &Device,
	                                 std::span<const vk::DescriptorSetLayoutBinding> Bindings) WIND_NOEXCEPT
	    -> TWindResult<void>;

	WIND_NODISCARD auto GetPool() const -> const vk::raii::DescriptorPool *;
	WIND_NODISCARD auto GetSet() const -> const vk::raii::DescriptorSet *;
	WIND_NODISCARD auto GetLayout() const -> const vk::raii::DescriptorSetLayout *;

	WIND_NODISCARD auto RegisterTexture(const vk::raii::Device &Device, const FAllocatedTexture &Texture) WIND_NOEXCEPT
	    -> uint32
	{
		vk::DescriptorImageInfo ImageInfo{};
		ImageInfo.imageView = Texture.Image.ImageView;
		ImageInfo.sampler = Texture.Sampler;
		ImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		const uint32 Index = MIndex++;

		spdlog::info("current index for bindless descriptor set: {}", Index);

		vk::WriteDescriptorSet Write{};
		Write.descriptorCount = 1;
		Write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		Write.dstBinding = 0;
		Write.dstArrayElement = Index;
		Write.pImageInfo = &ImageInfo;
		Write.dstSet = MSet;

		Device.updateDescriptorSets(Write, {});

		return Index;
	}

  private:
	explicit FUDescriptorManager(vk::raii::DescriptorPool Pool) : MPool{std::move(Pool)} {};

	vk::raii::DescriptorPool MPool{nullptr};
	vk::raii::DescriptorSet MSet{nullptr};
	vk::raii::DescriptorSetLayout MLayout{nullptr};
	uint32 MIndex{};
};
