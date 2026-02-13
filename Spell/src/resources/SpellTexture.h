#pragma once

#include "core/SpellDevice.h"
#include <string>

namespace Spell {

class SpellTexture {
public:
	SpellTexture(SpellDevice& device, const std::string& texturePath);
	SpellTexture(SpellDevice& device); // creates 1x1 white fallback texture
	~SpellTexture();

	SpellTexture(const SpellTexture&) = delete;
	SpellTexture& operator=(const SpellTexture&) = delete;

	VkImageView getImageView() const { return textureImageView_; }
	VkSampler getSampler() const { return textureSampler_; }
	uint32_t getMipLevels() const { return mipLevels_; }

private:
	void createTextureImage(const std::string& texturePath);
	void createWhiteTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	SpellDevice& device_;

	uint32_t mipLevels_;
	VkImage textureImage_;
	VkDeviceMemory textureImageMemory_;
	VkImageView textureImageView_;
	VkSampler textureSampler_;
};

} // namespace Spell
