#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "SpellTexture.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace Spell {

// ============================================================
// Deferred mode constructors
// ============================================================

SpellTexture::SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb, bool deferred)
	: device_(device), srgb_(srgb), deferred_(deferred) {
	prepareTextureImage(texturePath);
	createTextureImageView();
	createTextureSampler();
	if (!deferred_) {
		// Immediate: submit right away (legacy path)
		VkCommandBuffer cmd = device_.beginSingleTimeCommands();
		recordUpload(cmd);
		if (needsMipmaps_) {
			recordMipmaps(cmd);
		}
		device_.endSingleTimeCommands(cmd);
		finalizeStagingCleanup();
	}
}

SpellTexture::SpellTexture(SpellDevice& device, bool srgb, bool deferred)
	: device_(device), srgb_(srgb), deferred_(deferred) {
	prepareFallbackTextureImage();
	createTextureImageView();
	createTextureSampler();
	if (!deferred_) {
		VkCommandBuffer cmd = device_.beginSingleTimeCommands();
		recordUpload(cmd);
		device_.endSingleTimeCommands(cmd);
		finalizeStagingCleanup();
	}
}

SpellTexture::SpellTexture(SpellDevice& device, bool srgb, bool deferred, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	: device_(device), srgb_(srgb), deferred_(deferred) {
	prepareCustomColorImage(r, g, b, a);
	createTextureImageView();
	createTextureSampler();
	if (!deferred_) {
		VkCommandBuffer cmd = device_.beginSingleTimeCommands();
		recordUpload(cmd);
		device_.endSingleTimeCommands(cmd);
		finalizeStagingCleanup();
	}
}

// ============================================================
// Deferred mode constructor with pre-decoded pixel data
// ============================================================

SpellTexture::SpellTexture(SpellDevice& device, const DecodedImageData& decoded, bool srgb, bool deferred)
	: device_(device), srgb_(srgb), deferred_(deferred) {
	prepareFromDecodedData(decoded);
	createTextureImageView();
	createTextureSampler();
	if (!deferred_) {
		VkCommandBuffer cmd = device_.beginSingleTimeCommands();
		recordUpload(cmd);
		if (needsMipmaps_) {
			recordMipmaps(cmd);
		}
		device_.endSingleTimeCommands(cmd);
		finalizeStagingCleanup();
	}
}

// ============================================================
// Legacy immediate mode constructors
// ============================================================

SpellTexture::SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb)
	: SpellTexture(device, texturePath, srgb, false) {}

SpellTexture::SpellTexture(SpellDevice& device, bool srgb)
	: SpellTexture(device, srgb, false) {}

SpellTexture::SpellTexture(SpellDevice& device, bool srgb, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	: SpellTexture(device, srgb, false, r, g, b, a) {}

// ============================================================
// Destructor
// ============================================================

SpellTexture::~SpellTexture() {
	finalizeStagingCleanup();
	vkDestroySampler(device_.device(), textureSampler_, nullptr);
	vkDestroyImageView(device_.device(), textureImageView_, nullptr);
	vkDestroyImage(device_.device(), textureImage_, nullptr);
	vkFreeMemory(device_.device(), textureImageMemory_, nullptr);
}

// ============================================================
// Prepare: CPU-side decode + staging buffer + GPU image creation (no submit)
// ============================================================

void SpellTexture::prepareTextureImage(const std::string& texturePath) {
	int texChannels;
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth_, &texHeight_, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth_ * texHeight_ * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image: " + texturePath);
	}

	mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth_, texHeight_)))) + 1;
	needsMipmaps_ = (mipLevels_ > 1);
	VkFormat format = getFormat();

	device_.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer_, stagingBufferMemory_);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory_, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device_.device(), stagingBufferMemory_);

	stbi_image_free(pixels);

	device_.createImage(texWidth_, texHeight_, mipLevels_, VK_SAMPLE_COUNT_1_BIT, format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
}

void SpellTexture::prepareFromDecodedData(const DecodedImageData& decoded) {
	texWidth_ = decoded.width;
	texHeight_ = decoded.height;

	mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth_, texHeight_)))) + 1;
	needsMipmaps_ = (mipLevels_ > 1);
	VkFormat format = getFormat();

	device_.createBuffer(decoded.imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer_, stagingBufferMemory_);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory_, 0, decoded.imageSize, 0, &data);
	memcpy(data, decoded.pixels, static_cast<size_t>(decoded.imageSize));
	vkUnmapMemory(device_.device(), stagingBufferMemory_);

	device_.createImage(texWidth_, texHeight_, mipLevels_, VK_SAMPLE_COUNT_1_BIT, format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
}

void SpellTexture::prepareFallbackTextureImage() {
	mipLevels_ = 1;
	texWidth_ = 1;
	texHeight_ = 1;
	needsMipmaps_ = false;
	VkFormat format = getFormat();

	const unsigned char srgbPixel[4] = { 255, 255, 255, 255 };
	const unsigned char unormPixel[4] = { 128, 128, 255, 255 };
	const unsigned char* pixel = srgb_ ? srgbPixel : unormPixel;
	VkDeviceSize imageSize = 4;

	device_.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer_, stagingBufferMemory_);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory_, 0, imageSize, 0, &data);
	memcpy(data, pixel, imageSize);
	vkUnmapMemory(device_.device(), stagingBufferMemory_);

	device_.createImage(texWidth_, texHeight_, mipLevels_, VK_SAMPLE_COUNT_1_BIT, format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
}

void SpellTexture::prepareCustomColorImage(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	mipLevels_ = 1;
	texWidth_ = 1;
	texHeight_ = 1;
	needsMipmaps_ = false;
	VkFormat format = getFormat();

	const unsigned char pixel[4] = { r, g, b, a };
	VkDeviceSize imageSize = 4;

	device_.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer_, stagingBufferMemory_);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory_, 0, imageSize, 0, &data);
	memcpy(data, pixel, imageSize);
	vkUnmapMemory(device_.device(), stagingBufferMemory_);

	device_.createImage(texWidth_, texHeight_, mipLevels_, VK_SAMPLE_COUNT_1_BIT, format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
}

// ============================================================
// Record GPU commands (for batched submission)
// ============================================================

void SpellTexture::recordUpload(VkCommandBuffer cmd) {
	if (stagingBuffer_ == VK_NULL_HANDLE) return;

	VkFormat format = getFormat();

	// Transition UNDEFINED -> TRANSFER_DST_OPTIMAL
	device_.cmdTransitionImageLayout(cmd, textureImage_, format,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels_);

	// Copy staging buffer to image
	device_.cmdCopyBufferToImage(cmd, stagingBuffer_, textureImage_,
		static_cast<uint32_t>(texWidth_), static_cast<uint32_t>(texHeight_));

	// If no mipmaps needed, transition directly to SHADER_READ_ONLY
	if (!needsMipmaps_) {
		device_.cmdTransitionImageLayout(cmd, textureImage_, format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels_);
	}
	// If mipmaps needed, leave in TRANSFER_DST_OPTIMAL for recordMipmaps()
}

void SpellTexture::recordMipmaps(VkCommandBuffer cmd) {
	if (!needsMipmaps_) return;

	VkFormat imageFormat = getFormat();
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(device_.physicalDevice(), imageFormat, &formatProperties);
	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = textureImage_;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth_;
	int32_t mipHeight = texHeight_;

	for (uint32_t i = 1; i < mipLevels_; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmd,
			textureImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			textureImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels_ - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier);
}

void SpellTexture::finalizeStagingCleanup() {
	if (stagingBuffer_ != VK_NULL_HANDLE) {
		vkDestroyBuffer(device_.device(), stagingBuffer_, nullptr);
		stagingBuffer_ = VK_NULL_HANDLE;
	}
	if (stagingBufferMemory_ != VK_NULL_HANDLE) {
		vkFreeMemory(device_.device(), stagingBufferMemory_, nullptr);
		stagingBufferMemory_ = VK_NULL_HANDLE;
	}
}

// ============================================================
// Legacy immediate mode methods (kept for old code paths like SpellModel vertex buffer)
// ============================================================

void SpellTexture::createTextureImage(const std::string& texturePath) {
	prepareTextureImage(texturePath);
	VkCommandBuffer cmd = device_.beginSingleTimeCommands();
	recordUpload(cmd);
	if (needsMipmaps_) recordMipmaps(cmd);
	device_.endSingleTimeCommands(cmd);
	finalizeStagingCleanup();
}

void SpellTexture::createFallbackTextureImage() {
	prepareFallbackTextureImage();
	VkCommandBuffer cmd = device_.beginSingleTimeCommands();
	recordUpload(cmd);
	device_.endSingleTimeCommands(cmd);
	finalizeStagingCleanup();
}

void SpellTexture::createTextureImageView() {
	textureImageView_ = device_.createImageView(textureImage_, getFormat(), VK_IMAGE_ASPECT_COLOR_BIT, mipLevels_);
}

void SpellTexture::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties = device_.getProperties();
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipLevels_);

	if (vkCreateSampler(device_.device(), &samplerInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void SpellTexture::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	VkCommandBuffer commandBuffer = device_.beginSingleTimeCommands();
	texWidth_ = texWidth;
	texHeight_ = texHeight;
	mipLevels_ = mipLevels;
	needsMipmaps_ = true;
	recordMipmaps(commandBuffer);
	device_.endSingleTimeCommands(commandBuffer);
}

} // namespace Spell
