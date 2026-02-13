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
	auto totalStart = std::chrono::high_resolution_clock::now();

	auto modelStart = std::chrono::high_resolution_clock::now();
	model_ = std::make_unique<SpellModel>(device_, modelPath_);
	auto modelEnd = std::chrono::high_resolution_clock::now();
	lastModelLoadTimeMs_ = std::chrono::duration<float, std::milli>(modelEnd - modelStart).count();

	auto texStart = std::chrono::high_resolution_clock::now();
	textures_.clear();
	createFallbackWhiteTexture();
	loadMaterialTextures();
	submitBatchedTextureUpload();
	auto texEnd = std::chrono::high_resolution_clock::now();
	lastTextureLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - texStart).count();

	lastTotalLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - totalStart).count();

	std::cout << "[Spell] Load times - Model: " << lastModelLoadTimeMs_
		<< "ms, Textures: " << lastTextureLoadTimeMs_
		<< "ms, Total: " << lastTotalLoadTimeMs_ << "ms" << std::endl;
}

void SpellResourceManager::reloadResources() {
	vkDeviceWaitIdle(device_.device());

	model_.reset();
	textures_.clear();

	auto totalStart = std::chrono::high_resolution_clock::now();

	auto modelStart = std::chrono::high_resolution_clock::now();
	model_ = std::make_unique<SpellModel>(device_, modelPath_);
	auto modelEnd = std::chrono::high_resolution_clock::now();
	lastModelLoadTimeMs_ = std::chrono::duration<float, std::milli>(modelEnd - modelStart).count();

	auto texStart = std::chrono::high_resolution_clock::now();
	createFallbackWhiteTexture();
	loadMaterialTextures();
	submitBatchedTextureUpload();
	auto texEnd = std::chrono::high_resolution_clock::now();
	lastTextureLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - texStart).count();

	lastTotalLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - totalStart).count();

	std::cout << "[Spell] Reloaded model: " << modelPath_
		<< ", total textures: " << textures_.size()
		<< " - Load times - Model: " << lastModelLoadTimeMs_
		<< "ms, Textures: " << lastTextureLoadTimeMs_
		<< "ms, Total: " << lastTotalLoadTimeMs_ << "ms" << std::endl;
}

void SpellResourceManager::createFallbackWhiteTexture() {
	// Slot 0: fallback diffuse (sRGB white)
	try {
		textures_.push_back(std::make_unique<SpellTexture>(device_, texturePath_, true, true));
		std::cout << "[Spell] Loaded fallback diffuse texture: " << texturePath_ << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "[Spell] Failed to load fallback texture '" << texturePath_
			<< "', creating white 1x1: " << e.what() << std::endl;
		textures_.push_back(std::make_unique<SpellTexture>(device_, true, true));
	}

	// Slot 1: fallback normal (UNORM, default up-facing normal)
	textures_.push_back(std::make_unique<SpellTexture>(device_, false, true));
	std::cout << "[Spell] Created fallback normal texture (128,128,255)" << std::endl;

	// Slot 2: fallback metallic (UNORM, black = non-metallic)
	textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 0, 0, 0, 255));
	std::cout << "[Spell] Created fallback metallic texture (0,0,0) = non-metallic" << std::endl;

	// Slot 3: fallback roughness (UNORM, mid-gray = 0.5 roughness)
	textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 128, 128, 128, 255));
	std::cout << "[Spell] Created fallback roughness texture (128,128,128) = 0.5 roughness" << std::endl;
}

void SpellResourceManager::loadMaterialTextures() {
	if (!model_) return;

	const auto& materials = model_->getMaterials();
	for (size_t i = 0; i < materials.size(); i++) {
		const auto& mat = materials[i];

		// Diffuse texture (sRGB)
		if (!mat.diffuseTexturePath.empty() && std::filesystem::exists(mat.diffuseTexturePath)) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(device_, mat.diffuseTexturePath, true, true));
				std::cout << "[Spell] Loaded material[" << i << "] diffuse: " << mat.diffuseTexturePath << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to load material[" << i << "] diffuse: " << e.what() << std::endl;
				textures_.push_back(std::make_unique<SpellTexture>(device_, true, true));
			}
		} else {
			std::cout << "[Spell] Material[" << i << "] has no diffuse texture, using white fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, true, true));
		}

		// Normal texture (UNORM, linear)
		if (!mat.normalTexturePath.empty() && std::filesystem::exists(mat.normalTexturePath)) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(device_, mat.normalTexturePath, false, true));
				std::cout << "[Spell] Loaded material[" << i << "] normal: " << mat.normalTexturePath << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to load material[" << i << "] normal: " << e.what() << std::endl;
				textures_.push_back(std::make_unique<SpellTexture>(device_, false, true));
			}
		} else {
			std::cout << "[Spell] Material[" << i << "] has no normal texture, using default normal" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true));
		}

		// Metallic texture (UNORM, linear)
		if (!mat.metallicTexturePath.empty() && std::filesystem::exists(mat.metallicTexturePath)) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(device_, mat.metallicTexturePath, false, true));
				std::cout << "[Spell] Loaded material[" << i << "] metallic: " << mat.metallicTexturePath << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to load material[" << i << "] metallic: " << e.what() << std::endl;
				textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 0, 0, 0, 255));
			}
		} else {
			std::cout << "[Spell] Material[" << i << "] has no metallic texture, using black fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 0, 0, 0, 255));
		}

		// Roughness texture (UNORM, linear)
		if (!mat.roughnessTexturePath.empty() && std::filesystem::exists(mat.roughnessTexturePath)) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(device_, mat.roughnessTexturePath, false, true));
				std::cout << "[Spell] Loaded material[" << i << "] roughness: " << mat.roughnessTexturePath << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to load material[" << i << "] roughness: " << e.what() << std::endl;
				textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 128, 128, 128, 255));
			}
		} else {
			std::cout << "[Spell] Material[" << i << "] has no roughness texture, using mid-gray fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 128, 128, 128, 255));
		}
	}

	std::cout << "[Spell] Total texture slots: " << textures_.size()
		<< " (" << TEXTURES_PER_MATERIAL << " fallback + " << materials.size() << " materials x " << TEXTURES_PER_MATERIAL << " slots)" << std::endl;
}

void SpellResourceManager::submitBatchedTextureUpload() {
	if (textures_.empty()) return;

	VkCommandBuffer cmd = device_.beginSingleTimeCommands();

	// Phase 1: Record all uploads (transition + copy)
	for (auto& tex : textures_) {
		tex->recordUpload(cmd);
	}

	// Phase 2: Record all mipmap generations
	for (auto& tex : textures_) {
		if (tex->needsMipmaps()) {
			tex->recordMipmaps(cmd);
		}
	}

	// Single submit + wait
	device_.endSingleTimeCommands(cmd);

	// Clean up all staging buffers
	for (auto& tex : textures_) {
		tex->finalizeStagingCleanup();
	}

	std::cout << "[Spell] Batched texture upload: " << textures_.size() << " textures in 1 submit" << std::endl;
}

void SpellResourceManager::scanAvailableFiles() {
	availableModels_.clear();
	availableTextures_.clear();

	if (std::filesystem::exists("assets")) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
			if (!entry.is_regular_file()) continue;
			auto ext = entry.path().extension().string();
			if (ext == ".obj" || ext == ".OBJ") {
				availableModels_.push_back(entry.path().generic_string());
			}
			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
				ext == ".gif" || ext == ".bmp" || ext == ".tga" ||
				ext == ".PNG" || ext == ".JPG" || ext == ".JPEG") {
				availableTextures_.push_back(entry.path().generic_string());
			}
		}
	}

	std::sort(availableModels_.begin(), availableModels_.end());
	std::sort(availableTextures_.begin(), availableTextures_.end());
}

} // namespace Spell
