#include "SpellResourceManager.h"

#include <algorithm>
#include <iostream>
#include <filesystem>

namespace Spell {

SpellResourceManager::SpellResourceManager(SpellDevice& device)
	: device_{ device } {
	scanAvailableFiles();
}

void SpellResourceManager::loadInitialResources() {
	model_ = std::make_unique<SpellModel>(device_, modelPath_);

	textures_.clear();
	createFallbackWhiteTexture();
	loadMaterialTextures();
}

void SpellResourceManager::reloadResources() {
	vkDeviceWaitIdle(device_.device());

	model_.reset();
	textures_.clear();

	model_ = std::make_unique<SpellModel>(device_, modelPath_);

	createFallbackWhiteTexture();
	loadMaterialTextures();

	std::cout << "[Spell] Reloaded model: " << modelPath_
		<< ", total textures: " << textures_.size() << std::endl;
}

void SpellResourceManager::createFallbackWhiteTexture() {
	// Try loading the user-selected texture as fallback (index 0)
	// If a specific texture is selected in the inspector, use it
	// Otherwise create a programmatic white texture
	try {
		textures_.push_back(std::make_unique<SpellTexture>(device_, texturePath_));
		std::cout << "[Spell] Loaded fallback texture: " << texturePath_ << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[Spell] Failed to load fallback texture '" << texturePath_
			<< "', creating white 1x1: " << e.what() << std::endl;
		textures_.push_back(std::make_unique<SpellTexture>(device_));
	}
}

void SpellResourceManager::loadMaterialTextures() {
	if (!model_) return;

	const auto& materials = model_->getMaterials();
	for (size_t i = 0; i < materials.size(); i++) {
		const auto& mat = materials[i];
		if (!mat.diffuseTexturePath.empty() && std::filesystem::exists(mat.diffuseTexturePath)) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(device_, mat.diffuseTexturePath));
				std::cout << "[Spell] Loaded material[" << i << "] texture: " << mat.diffuseTexturePath << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to load material[" << i << "] texture: " << e.what() << std::endl;
				textures_.push_back(std::make_unique<SpellTexture>(device_));
			}
		} else {
			// No diffuse texture for this material, use a white fallback
			std::cout << "[Spell] Material[" << i << "] has no diffuse texture, using white fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_));
		}
	}

	std::cout << "[Spell] Total texture slots: " << textures_.size()
		<< " (1 fallback + " << materials.size() << " materials)" << std::endl;
}

void SpellResourceManager::scanAvailableFiles() {
	availableModels_.clear();
	availableTextures_.clear();

	if (std::filesystem::exists("models")) {
		for (const auto& entry : std::filesystem::directory_iterator("models")) {
			if (entry.is_regular_file()) {
				auto ext = entry.path().extension().string();
				if (ext == ".obj" || ext == ".OBJ") {
					availableModels_.push_back(entry.path().generic_string());
				}
			}
		}
	}

	if (std::filesystem::exists("textures")) {
		for (const auto& entry : std::filesystem::directory_iterator("textures")) {
			if (entry.is_regular_file()) {
				auto ext = entry.path().extension().string();
				if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
					ext == ".gif" || ext == ".bmp" || ext == ".tga" ||
					ext == ".PNG" || ext == ".JPG" || ext == ".JPEG") {
					availableTextures_.push_back(entry.path().generic_string());
				}
			}
		}
	}

	std::sort(availableModels_.begin(), availableModels_.end());
	std::sort(availableTextures_.begin(), availableTextures_.end());
}

} // namespace Spell
