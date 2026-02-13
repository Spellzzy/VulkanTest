#pragma once

#include "SpellModel.h"
#include "SpellTexture.h"

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace Spell {

static constexpr uint32_t MAX_BINDLESS_TEXTURES = 128;

class SpellResourceManager {
public:
	SpellResourceManager(SpellDevice& device);

	void scanAvailableFiles();

	const std::string& modelPath() const { return modelPath_; }
	const std::string& texturePath() const { return texturePath_; }
	void setModelPath(const std::string& path) { modelPath_ = path; }
	void setTexturePath(const std::string& path) { texturePath_ = path; }

	const std::vector<std::string>& availableModels() const { return availableModels_; }
	const std::vector<std::string>& availableTextures() const { return availableTextures_; }

	SpellModel* model() const { return model_.get(); }

	// Texture slots per material (diffuse + normal + metallic + roughness)
	static constexpr uint32_t TEXTURES_PER_MATERIAL = 4;

	// Bindless texture array: index 0,1 are fallback (diffuse, normal), then per-material slots
	const std::vector<std::unique_ptr<SpellTexture>>& textures() const { return textures_; }
	uint32_t textureCount() const { return static_cast<uint32_t>(textures_.size()); }

	// Legacy single texture access (for inspector display)
	SpellTexture* texture() const { return textures_.empty() ? nullptr : textures_[0].get(); }

	void loadInitialResources();
	void reloadResources();

	float lastModelLoadTimeMs() const { return lastModelLoadTimeMs_; }
	float lastTextureLoadTimeMs() const { return lastTextureLoadTimeMs_; }
	float lastTotalLoadTimeMs() const { return lastTotalLoadTimeMs_; }

private:
	void createFallbackWhiteTexture();
	void loadMaterialTextures();
	void submitBatchedTextureUpload();

	SpellDevice& device_;

	std::string modelPath_{ "assets/viking_room/viking_room.obj" };
	std::string texturePath_{ "assets/viking_room/viking_room.png" };
	std::vector<std::string> availableModels_;
	std::vector<std::string> availableTextures_;

	std::unique_ptr<SpellModel> model_;
	std::vector<std::unique_ptr<SpellTexture>> textures_; // [0] = fallback, [1..N] = material textures

	float lastModelLoadTimeMs_ = 0.0f;
	float lastTextureLoadTimeMs_ = 0.0f;
	float lastTotalLoadTimeMs_ = 0.0f;
};

} // namespace Spell
