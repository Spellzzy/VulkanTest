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
	texture_ = std::make_unique<SpellTexture>(device_, texturePath_);
}

void SpellResourceManager::reloadResources() {
	vkDeviceWaitIdle(device_.device());

	model_.reset();
	texture_.reset();

	model_ = std::make_unique<SpellModel>(device_, modelPath_);
	texture_ = std::make_unique<SpellTexture>(device_, texturePath_);

	std::cout << "[Spell] Reloaded model: " << modelPath_
		<< ", texture: " << texturePath_ << std::endl;
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
