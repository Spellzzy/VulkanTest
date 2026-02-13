#pragma once

#include "core/SpellWindow.h"
#include "core/SpellDevice.h"
#include "core/SpellSwapChain.h"

#include <vector>
#include <memory>
#include <cassert>

namespace Spell {

class SpellRenderer {
public:
	SpellRenderer(SpellWindow& window, SpellDevice& device);
	~SpellRenderer();

	SpellRenderer(const SpellRenderer&) = delete;
	SpellRenderer& operator=(const SpellRenderer&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return swapChain_->getRenderPass(); }
	float getAspectRatio() const { return swapChain_->extentAspectRatio(); }
	VkExtent2D getSwapChainExtent() const { return swapChain_->getSwapChainExtent(); }
	bool isFrameInProgress() const { return isFrameStarted_; }
	size_t getSwapChainImageCount() const { return swapChain_->imageCount(); }

	VkCommandBuffer getCurrentCommandBuffer() const {
		assert(isFrameStarted_ && "Cannot get command buffer when frame not in progress");
		return commandBuffers_[currentFrameIndex_];
	}

	int getFrameIndex() const {
		assert(isFrameStarted_ && "Cannot get frame index when frame not in progress");
		return currentFrameIndex_;
	}

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginRenderPass(VkCommandBuffer commandBuffer);
	void endRenderPass(VkCommandBuffer commandBuffer);

private:
	void createCommandBuffers();
	void freeCommandBuffers();
	void recreateSwapChain();

	SpellWindow& window_;
	SpellDevice& device_;
	std::unique_ptr<SpellSwapChain> swapChain_;
	std::vector<VkCommandBuffer> commandBuffers_;

	uint32_t currentImageIndex_ = 0;
	int currentFrameIndex_ = 0;
	bool isFrameStarted_ = false;
};

} // namespace Spell
