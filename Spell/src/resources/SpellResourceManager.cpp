#include "SpellResourceManager.h"

#include <stb_image.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <future>

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

	// ========== Phase 1: Collect all texture tasks ==========
	struct TextureTask {
		std::string path;
		bool srgb;
		bool hasFile;
		// Fallback info for textures without a file
		enum Type { Diffuse, Normal, Metallic, Roughness } type;
	};

	std::vector<TextureTask> tasks;
	for (const auto& mat : materials) {
		tasks.push_back({ mat.diffuseTexturePath, true,
			!mat.diffuseTexturePath.empty() && std::filesystem::exists(mat.diffuseTexturePath),
			TextureTask::Diffuse });
		tasks.push_back({ mat.normalTexturePath, false,
			!mat.normalTexturePath.empty() && std::filesystem::exists(mat.normalTexturePath),
			TextureTask::Normal });
		tasks.push_back({ mat.metallicTexturePath, false,
			!mat.metallicTexturePath.empty() && std::filesystem::exists(mat.metallicTexturePath),
			TextureTask::Metallic });
		tasks.push_back({ mat.roughnessTexturePath, false,
			!mat.roughnessTexturePath.empty() && std::filesystem::exists(mat.roughnessTexturePath),
			TextureTask::Roughness });
	}

	// ========== Phase 2: Parallel CPU decode (stbi_load is thread-safe) ==========
	auto decodeImage = [](const std::string& path) -> DecodedImageData {
		DecodedImageData result;
		result.sourcePath = path;
		int texChannels;
		result.pixels = stbi_load(path.c_str(), &result.width, &result.height, &texChannels, STBI_rgb_alpha);
		if (result.pixels) {
			result.imageSize = static_cast<VkDeviceSize>(result.width) * result.height * 4;
			result.valid = true;
		}
		return result;
	};

	std::vector<std::future<DecodedImageData>> futures(tasks.size());
	for (size_t i = 0; i < tasks.size(); i++) {
		if (tasks[i].hasFile) {
			futures[i] = std::async(std::launch::async, decodeImage, tasks[i].path);
		}
	}

	// ========== Phase 3: Collect results on main thread, create SpellTexture ==========
	for (size_t i = 0; i < tasks.size(); i++) {
		size_t matIdx = i / TEXTURES_PER_MATERIAL;
		const auto& task = tasks[i];

		if (task.hasFile && futures[i].valid()) {
			DecodedImageData decoded = futures[i].get();
			if (decoded.valid) {
				try {
					textures_.push_back(std::make_unique<SpellTexture>(
						device_, decoded, task.srgb, true));
					std::cout << "[Spell] Loaded material[" << matIdx << "] "
						<< (task.type == TextureTask::Diffuse ? "diffuse" :
							task.type == TextureTask::Normal ? "normal" :
							task.type == TextureTask::Metallic ? "metallic" : "roughness")
						<< ": " << task.path << std::endl;
					stbi_image_free(decoded.pixels);
					continue;
				} catch (const std::exception& e) {
					std::cerr << "[Spell] Failed to create texture from decoded data: " << e.what() << std::endl;
					stbi_image_free(decoded.pixels);
				}
			} else {
				std::cerr << "[Spell] Failed to decode material[" << matIdx << "] texture: " << task.path << std::endl;
			}
		}

		// Fallback path
		switch (task.type) {
		case TextureTask::Diffuse:
			std::cout << "[Spell] Material[" << matIdx << "] has no diffuse texture, using white fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, true, true));
			break;
		case TextureTask::Normal:
			std::cout << "[Spell] Material[" << matIdx << "] has no normal texture, using default normal" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true));
			break;
		case TextureTask::Metallic:
			std::cout << "[Spell] Material[" << matIdx << "] has no metallic texture, using black fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 0, 0, 0, 255));
			break;
		case TextureTask::Roughness:
			std::cout << "[Spell] Material[" << matIdx << "] has no roughness texture, using mid-gray fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 128, 128, 128, 255));
			break;
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
