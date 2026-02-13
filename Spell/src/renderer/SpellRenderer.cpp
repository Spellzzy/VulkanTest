#include "SpellRenderer.h"

#include <stdexcept>
#include <array>

namespace Spell {

SpellRenderer::SpellRenderer(SpellWindow& window, SpellDevice& device)
	: window_(window), device_(device) {
	recreateSwapChain();
	createCommandBuffers();
}

SpellRenderer::~SpellRenderer() {
	freeCommandBuffers();
}

void SpellRenderer::createCommandBuffers() {
	commandBuffers_.resize(SpellSwapChain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device_.commandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

	if (vkAllocateCommandBuffers(device_.device(), &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void SpellRenderer::freeCommandBuffers() {
	vkFreeCommandBuffers(device_.device(), device_.commandPool(),
		static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
	commandBuffers_.clear();
}

void SpellRenderer::recreateSwapChain() {
	auto extent = window_.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = window_.getExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device_.device());

	if (swapChain_ == nullptr) {
		swapChain_ = std::make_unique<SpellSwapChain>(device_, extent);
	} else {
		std::shared_ptr<SpellSwapChain> oldSwapChain = std::move(swapChain_);
		swapChain_ = std::make_unique<SpellSwapChain>(device_, extent, oldSwapChain);
	}
}

VkCommandBuffer SpellRenderer::beginFrame() {
	assert(!isFrameStarted_ && "Can't call beginFrame while already in progress");

	auto result = swapChain_->acquireNextImage(&currentImageIndex_);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	isFrameStarted_ = true;
	auto commandBuffer = getCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	return commandBuffer;
}

void SpellRenderer::endFrame() {
	assert(isFrameStarted_ && "Can't call endFrame while frame is not in progress");

	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	auto result = swapChain_->submitCommandBuffers(&commandBuffer, &currentImageIndex_);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.wasResized()) {
		window_.resetResizedFlag();
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	isFrameStarted_ = false;
	currentFrameIndex_ = (currentFrameIndex_ + 1) % SpellSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void SpellRenderer::beginRenderPass(VkCommandBuffer commandBuffer) {
	assert(isFrameStarted_ && "Can't call beginRenderPass while frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapChain_->getRenderPass();
	renderPassInfo.framebuffer = swapChain_->getFramebuffer(currentImageIndex_);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain_->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.01f, 0.01f, 0.01f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain_->getSwapChainExtent().width);
	viewport.height = static_cast<float>(swapChain_->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain_->getSwapChainExtent();
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void SpellRenderer::endRenderPass(VkCommandBuffer commandBuffer) {
	assert(isFrameStarted_ && "Can't call endRenderPass while frame is not in progress");
	assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}

} // namespace Spell
