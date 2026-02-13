#pragma once

#include "core/SpellWindow.h"
#include <vector>
#include <optional>

namespace Spell {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class SpellDevice {
public:
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	SpellDevice(SpellWindow& window);
	~SpellDevice();

	SpellDevice(const SpellDevice&) = delete;
	SpellDevice& operator=(const SpellDevice&) = delete;

	VkDevice device() { return device_; }
	VkPhysicalDevice physicalDevice() { return physicalDevice_; }
	VkCommandPool commandPool() { return commandPool_; }
	VkQueue graphicsQueue() { return graphicsQueue_; }
	VkQueue presentQueue() { return presentQueue_; }
	VkSurfaceKHR surface() { return surface_; }
	VkInstance getInstance() { return instance_; }
	VkSampleCountFlagBits msaaSamples() { return msaaSamples_; }

	SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice_); }
	QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice_); }
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	// Command buffer recording variants (no submit, for batched operations)
	void cmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void cmdCopyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDeviceSize bufferOffset);
	void cmdTransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	VkPhysicalDeviceProperties getProperties() {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice_, &props);
		return props;
	}

private:
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool();

	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	bool hasStencilComponent(VkFormat format);

	VkInstance instance_;
	VkDevice device_;
	VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
	VkPhysicalDeviceFeatures deviceFeatures_{};
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures_{};
	VkSurfaceKHR surface_;
	VkCommandPool commandPool_;
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;
	VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;

	SpellWindow& window_;

	const std::vector<const char*> validationLayers_ = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> deviceExtensions_ = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

} // namespace Spell
