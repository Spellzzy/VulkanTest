#pragma once

#include "core/SpellDevice.h"
#include <string>

namespace Spell {

class SpellTexture {
public:
	SpellTexture(SpellDevice& device, const std::string& texturePath, bool srgb = true);
	SpellTexture(SpellDevice& device, bool srgb = true); // creates 1x1 fallback texture
	SpellTexture(SpellDevice& device, bool srgb, unsigned char r, unsigned char g, unsigned char b, unsigned char a); // custom color fallback
	~SpellTexture();

	SpellTexture(const SpellTexture&) = delete;
	SpellTexture& operator=(const SpellTexture&) = delete;

	VkImageView getImageView() const { return textureImageView_; }
	VkSampler getSampler() const { return textureSampler_; }
	uint32_t getMipLevels() const { return mipLevels_; }

private:
	void createTextureImage(const std::string& texturePath);
	void createFallbackTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VkFormat getFormat() const { return srgb_ ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; }

	SpellDevice& device_;
	bool srgb_;

	uint32_t mipLevels_;
	VkImage textureImage_;
	VkDeviceMemory textureImageMemory_;
	VkImageView textureImageView_;
	VkSampler textureSampler_;
};

} // namespace Spell
