#pragma once

#include "core/SpellDevice.h"
#include <string>
#include <vector>

namespace Spell {

class SpellTexture {
public:
	// Deferred mode: prepares CPU-side data and GPU image, but does NOT submit GPU commands.
	// Call recordUpload() and recordMipmaps() to record into a batched command buffer,
	// then call finalizeStagingCleanup() after the batch is submitted and completed.
	SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb, bool deferred);
	SpellTexture(SpellDevice& device, bool srgb, bool deferred);
	SpellTexture(SpellDevice& device, bool srgb, bool deferred, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

	// Legacy immediate mode constructors (kept for compatibility)
	SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb = true);
	SpellTexture(SpellDevice& device, bool srgb = true);
	SpellTexture(SpellDevice& device, bool srgb, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

	~SpellTexture();

	SpellTexture(const SpellTexture&) = delete;
	SpellTexture& operator=(const SpellTexture&) = delete;

	VkImageView getImageView() const { return textureImageView_; }
	VkSampler getSampler() const { return textureSampler_; }
	uint32_t getMipLevels() const { return mipLevels_; }

	// Record GPU upload commands into an external command buffer (deferred mode)
	void recordUpload(VkCommandBuffer cmd);

	// Record mipmap generation commands into an external command buffer (deferred mode)
	void recordMipmaps(VkCommandBuffer cmd);

	// Clean up staging buffers after GPU work is complete
	void finalizeStagingCleanup();

	bool needsMipmaps() const { return needsMipmaps_; }

private:
	void prepareTextureImage(const std::string& texturePath);
	void prepareFallbackTextureImage();
	void prepareCustomColorImage(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

	void createTextureImage(const std::string& texturePath);
	void createFallbackTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VkFormat getFormat() const { return srgb_ ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; }

	SpellDevice& device_;
	bool srgb_;

	uint32_t mipLevels_ = 1;
	VkImage textureImage_ = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory_ = VK_NULL_HANDLE;
	VkImageView textureImageView_ = VK_NULL_HANDLE;
	VkSampler textureSampler_ = VK_NULL_HANDLE;

	// Deferred upload state
	VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory_ = VK_NULL_HANDLE;
	int32_t texWidth_ = 0;
	int32_t texHeight_ = 0;
	bool needsMipmaps_ = false;
	bool deferred_ = false;
};

} // namespace Spell
