#pragma once

#include "core/SpellDevice.h"
#include <string>
#include <vector>

namespace Spell {

// Pre-decoded image data from CPU-side stbi_load (thread-safe, no Vulkan calls)
struct DecodedImageData {
	unsigned char* pixels = nullptr;
	int width = 0;
	int height = 0;
	VkDeviceSize imageSize = 0;
	bool valid = false;
	std::string sourcePath;
};

class SpellTexture {
public:
	// Deferred mode: prepares CPU-side data and GPU image, but does NOT submit GPU commands.
	// Call recordUpload() and recordMipmaps() to record into a batched command buffer,
	// then call finalizeStagingCleanup() after the batch is submitted and completed.
	SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb, bool deferred);
	SpellTexture(SpellDevice& device, bool srgb, bool deferred);
	SpellTexture(SpellDevice& device, bool srgb, bool deferred, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

	// Deferred mode: accepts pre-decoded pixel data (from parallel CPU decode)
	// Caller is responsible for stbi_image_free(pixels) after this constructor returns.
	SpellTexture(SpellDevice& device, const DecodedImageData& decoded, bool srgb, bool deferred);

	// Deferred mode: uses shared staging buffer (no individual staging allocation)
	// The shared staging buffer is managed externally by the caller.
	SpellTexture(SpellDevice& device, const DecodedImageData& decoded, bool srgb, bool deferred,
		VkBuffer sharedStagingBuffer, VkDeviceSize bufferOffset);

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
	void prepareFromDecodedData(const DecodedImageData& decoded);
	void prepareImageOnly(const DecodedImageData& decoded);
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
	bool ownsStaging_ = true;  // false when using shared staging buffer
	VkDeviceSize stagingBufferOffset_ = 0;
	int32_t texWidth_ = 0;
	int32_t texHeight_ = 0;
	bool needsMipmaps_ = false;
	bool deferred_ = false;
};

} // namespace Spell
