#pragma once

#include "core/SpellDevice.h"
#include <vector>
#include <memory>

namespace Spell {

class SpellSwapChain {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	SpellSwapChain(SpellDevice& device, VkExtent2D windowExtent);
	SpellSwapChain(SpellDevice& device, VkExtent2D windowExtent, std::shared_ptr<SpellSwapChain> previous);
	~SpellSwapChain();

	SpellSwapChain(const SpellSwapChain&) = delete;
	SpellSwapChain& operator=(const SpellSwapChain&) = delete;

	VkFramebuffer getFramebuffer(int index) { return swapChainFramebuffers_[index]; }
	VkRenderPass getRenderPass() { return renderPass_; }
	VkImageView getImageView(int index) { return swapChainImageViews_[index]; }
	size_t imageCount() { return swapChainImages_.size(); }
	VkFormat getSwapChainImageFormat() { return swapChainImageFormat_; }
	VkExtent2D getSwapChainExtent() { return swapChainExtent_; }
	uint32_t width() { return swapChainExtent_.width; }
	uint32_t height() { return swapChainExtent_.height; }

	float extentAspectRatio() {
		return static_cast<float>(swapChainExtent_.width) / static_cast<float>(swapChainExtent_.height);
	}

	VkFormat findDepthFormat();

	VkResult acquireNextImage(uint32_t* imageIndex);
	VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

private:
	void init();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createColorResources();
	void createDepthResources();
	void createFramebuffers();
	void createSyncObjects();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkFormat swapChainImageFormat_;
	VkExtent2D swapChainExtent_;

	VkRenderPass renderPass_;
	VkSwapchainKHR swapChain_;

	std::vector<VkFramebuffer> swapChainFramebuffers_;
	std::vector<VkImage> swapChainImages_;
	std::vector<VkImageView> swapChainImageViews_;

	// Depth resources
	VkImage depthImage_;
	VkDeviceMemory depthImageMemory_;
	VkImageView depthImageView_;

	// MSAA color resources
	VkImage colorImage_;
	VkDeviceMemory colorImageMemory_;
	VkImageView colorImageView_;

	// Sync objects
	std::vector<VkSemaphore> imageAvailableSemaphores_;
	std::vector<VkSemaphore> renderFinishedSemaphores_;
	std::vector<VkFence> inFlightFences_;
	std::vector<VkFence> imagesInFlight_;
	size_t currentFrame_ = 0;

	SpellDevice& device_;
	VkExtent2D windowExtent_;

	std::shared_ptr<SpellSwapChain> oldSwapChain_;
};

} // namespace Spell
