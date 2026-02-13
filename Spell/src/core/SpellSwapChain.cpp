#include "SpellSwapChain.h"

#include <iostream>
#include <array>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

namespace Spell {

SpellSwapChain::SpellSwapChain(SpellDevice& device, VkExtent2D windowExtent)
	: device_(device), windowExtent_(windowExtent) {
	init();
}

SpellSwapChain::SpellSwapChain(SpellDevice& device, VkExtent2D windowExtent, std::shared_ptr<SpellSwapChain> previous)
	: device_(device), windowExtent_(windowExtent), oldSwapChain_(previous) {
	init();
	oldSwapChain_ = nullptr;
}

void SpellSwapChain::init() {
	createSwapChain();
	createImageViews();
	createRenderPass();
	createColorResources();
	createDepthResources();
	createFramebuffers();
	createSyncObjects();
}

SpellSwapChain::~SpellSwapChain() {
	for (auto imageView : swapChainImageViews_) {
		vkDestroyImageView(device_.device(), imageView, nullptr);
	}
	swapChainImageViews_.clear();

	if (swapChain_ != nullptr) {
		vkDestroySwapchainKHR(device_.device(), swapChain_, nullptr);
		swapChain_ = nullptr;
	}

	// Depth resources
	vkDestroyImageView(device_.device(), depthImageView_, nullptr);
	vkDestroyImage(device_.device(), depthImage_, nullptr);
	vkFreeMemory(device_.device(), depthImageMemory_, nullptr);

	// Color (MSAA) resources
	vkDestroyImageView(device_.device(), colorImageView_, nullptr);
	vkDestroyImage(device_.device(), colorImage_, nullptr);
	vkFreeMemory(device_.device(), colorImageMemory_, nullptr);

	for (auto framebuffer : swapChainFramebuffers_) {
		vkDestroyFramebuffer(device_.device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(device_.device(), renderPass_, nullptr);

	for (size_t i = 0; i < imageAvailableSemaphores_.size(); i++) {
		vkDestroySemaphore(device_.device(), imageAvailableSemaphores_[i], nullptr);
	}
	for (size_t i = 0; i < renderFinishedSemaphores_.size(); i++) {
		vkDestroySemaphore(device_.device(), renderFinishedSemaphores_[i], nullptr);
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyFence(device_.device(), inFlightFences_[i], nullptr);
	}
}

void SpellSwapChain::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = device_.getSwapChainSupport();

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = device_.surface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = device_.findPhysicalQueueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain_ == nullptr ? VK_NULL_HANDLE : oldSwapChain_->swapChain_;

	if (vkCreateSwapchainKHR(device_.device(), &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device_.device(), swapChain_, &imageCount, nullptr);
	swapChainImages_.resize(imageCount);
	vkGetSwapchainImagesKHR(device_.device(), swapChain_, &imageCount, swapChainImages_.data());

	swapChainImageFormat_ = surfaceFormat.format;
	swapChainExtent_ = extent;
}

void SpellSwapChain::createImageViews() {
	swapChainImageViews_.resize(swapChainImages_.size());
	for (size_t i = 0; i < swapChainImages_.size(); i++) {
		swapChainImageViews_[i] = device_.createImageView(
			swapChainImages_[i], swapChainImageFormat_, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void SpellSwapChain::createRenderPass() {
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = device_.msaaSamples();
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat_;
	colorAttachment.samples = device_.msaaSamples();
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat_;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPass{};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorAttachmentRef;
	subPass.pDepthStencilAttachment = &depthAttachmentRef;
	subPass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device_.device(), &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void SpellSwapChain::createColorResources() {
	VkFormat colorFormat = swapChainImageFormat_;
	device_.createImage(
		swapChainExtent_.width, swapChainExtent_.height, 1, device_.msaaSamples(), colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage_, colorImageMemory_);
	colorImageView_ = device_.createImageView(colorImage_, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void SpellSwapChain::createDepthResources() {
	VkFormat depthFormat = findDepthFormat();
	device_.createImage(
		swapChainExtent_.width, swapChainExtent_.height, 1, device_.msaaSamples(), depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage_, depthImageMemory_);
	depthImageView_ = device_.createImageView(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void SpellSwapChain::createFramebuffers() {
	swapChainFramebuffers_.resize(swapChainImageViews_.size());
	for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
		std::array<VkImageView, 3> attachments = {
			colorImageView_,
			depthImageView_,
			swapChainImageViews_[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass_;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent_.width;
		framebufferInfo.height = swapChainExtent_.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device_.device(), &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void SpellSwapChain::createSyncObjects() {
	imageAvailableSemaphores_.resize(imageCount());
	renderFinishedSemaphores_.resize(imageCount());
	inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight_.resize(imageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < imageCount(); i++) {
		if (vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device_.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects!");
		}
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateFence(device_.device(), &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects!");
		}
	}
}

VkResult SpellSwapChain::acquireNextImage(uint32_t* imageIndex) {
	vkWaitForFences(device_.device(), 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

	// Use a rotating index for imageAvailable semaphores to avoid reuse conflicts.
	// We pick the semaphore based on acquireIndex_ which cycles through all swapchain images.
	uint32_t semaphoreIndex = acquireIndex_;
	acquireIndex_ = (acquireIndex_ + 1) % static_cast<uint32_t>(imageCount());

	VkResult result = vkAcquireNextImageKHR(
		device_.device(), swapChain_, UINT64_MAX,
		imageAvailableSemaphores_[semaphoreIndex], VK_NULL_HANDLE, imageIndex);

	// Store which semaphore was used for this image so submitCommandBuffers can wait on it
	currentAcquireSemaphore_ = semaphoreIndex;
	return result;
}

VkResult SpellSwapChain::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) {
	if (imagesInFlight_[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device_.device(), 1, &imagesInFlight_[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	imagesInFlight_[*imageIndex] = inFlightFences_[currentFrame_];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentAcquireSemaphore_] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[*imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device_.device(), 1, &inFlightFences_[currentFrame_]);

	if (vkQueueSubmit(device_.graphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain_ };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = imageIndex;

	auto result = vkQueuePresentKHR(device_.presentQueue(), &presentInfo);

	currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
	return result;
}

VkSurfaceFormatKHR SpellSwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR SpellSwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SpellSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = windowExtent_;
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

VkFormat SpellSwapChain::findDepthFormat() {
	return device_.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace Spell
