#include "SpellResourceManager.h"
#include "IModelLoader.h"
#include "ModelLoaderFactory.h"

#include <stb_image.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <future>

namespace Spell {

SpellResourceManager::SpellResourceManager(SpellDevice& device)
	: device_{ device } {
	scanAvailableFiles();
}

// ============================================================
// Shared parallel load pipeline: used by both loadInitial and reload
// ============================================================
void SpellResourceManager::loadWithLoader(IModelLoader& loader) {
	auto totalStart = std::chrono::high_resolution_clock::now();

	// Step 1: Pre-parse texture paths (fast, format-specific)
	auto preParsedMaterials = loader.preParseTexturePaths(modelPath_);

	// Step 2: Kick off async texture CPU decode BEFORE model loading
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

	struct TextureTask {
		std::string path;
		bool srgb;
		bool hasFile;
	};
	std::vector<TextureTask> tasks;
	std::vector<bool> srgbFlags;
	std::vector<bool> hasFileFlags;

	for (const auto& mat : preParsedMaterials) {
		auto addTask = [&](const std::string& path, bool srgb) {
			bool has = !path.empty() && std::filesystem::exists(path);
			tasks.push_back({ path, srgb, has });
			srgbFlags.push_back(srgb);
			hasFileFlags.push_back(has);
		};
		addTask(mat.diffuseTexturePath, true);
		addTask(mat.normalTexturePath, false);
		addTask(mat.metallicTexturePath, false);
		addTask(mat.roughnessTexturePath, false);
	}

	// Launch async decode for all textures that have files
	auto decodeStart = std::chrono::high_resolution_clock::now();
	std::vector<std::future<DecodedImageData>> futures(tasks.size());
	for (size_t i = 0; i < tasks.size(); i++) {
		if (tasks[i].hasFile) {
			futures[i] = std::async(std::launch::async, decodeImage, tasks[i].path);
		}
	}

	// Step 3: Load model IN PARALLEL with texture decoding
	auto modelStart = std::chrono::high_resolution_clock::now();
	auto loadResult = loader.load(modelPath_);
	model_ = std::make_unique<SpellModel>(device_, std::move(loadResult));
	auto modelEnd = std::chrono::high_resolution_clock::now();
	lastModelLoadTimeMs_ = std::chrono::duration<float, std::milli>(modelEnd - modelStart).count();

	// Step 4: Create fallback textures (fast)
	auto texStart = std::chrono::high_resolution_clock::now();
	createFallbackWhiteTexture();

	// Step 5: Collect decoded results and create GPU resources
	loadMaterialTexturesFromDecoded(preParsedMaterials, futures, hasFileFlags, srgbFlags);

	auto decodeEnd = std::chrono::high_resolution_clock::now();
	float totalDecodeMs = std::chrono::duration<float, std::milli>(decodeEnd - decodeStart).count();

	// Step 6: Batched GPU upload
	submitBatchedTextureUpload();
	auto texEnd = std::chrono::high_resolution_clock::now();
	lastTextureLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - texStart).count();

	lastTotalLoadTimeMs_ = std::chrono::duration<float, std::milli>(texEnd - totalStart).count();

	// Compute overlap savings
	float overlapMs = std::min(lastModelLoadTimeMs_, totalDecodeMs);
	lastDecodeOverlapMs_ = overlapMs;

	std::cout << "[Spell] Load times - Model: " << lastModelLoadTimeMs_
		<< "ms, Textures: " << lastTextureLoadTimeMs_
		<< "ms, Total: " << lastTotalLoadTimeMs_
		<< "ms (parallel overlap saved ~" << lastDecodeOverlapMs_ << "ms)" << std::endl;
}

// ============================================================
// Parallel load: model loading and texture CPU decode run concurrently
// ============================================================

void SpellResourceManager::loadInitialResources() {
	auto loader = ModelLoaderFactory::createLoader(modelPath_);
	loadWithLoader(*loader);
}

void SpellResourceManager::reloadResources() {
	vkDeviceWaitIdle(device_.device());

	model_.reset();
	textures_.clear();

	auto loader = ModelLoaderFactory::createLoader(modelPath_);
	loadWithLoader(*loader);

	std::cout << "[Spell] Reloaded model: " << modelPath_
		<< ", total textures: " << textures_.size() << std::endl;
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

	// ========== Phase 2: Parallel CPU decode ==========
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

	// ========== Phase 3: Collect results ==========
	std::vector<bool> srgbFlags;
	std::vector<bool> hasFileFlags;
	for (const auto& t : tasks) {
		srgbFlags.push_back(t.srgb);
		hasFileFlags.push_back(t.hasFile);
	}

	loadMaterialTexturesFromDecoded(materials, futures, hasFileFlags, srgbFlags);
}

void SpellResourceManager::loadMaterialTexturesFromDecoded(
	const std::vector<MaterialInfo>& materials,
	std::vector<std::future<DecodedImageData>>& futures,
	const std::vector<bool>& hasFile,
	const std::vector<bool>& srgbFlags) {

	enum TextureType { Diffuse, Normal, Metallic, Roughness };

	// ========== Collect results, compute total staging size ==========
	struct ResolvedTexture {
		DecodedImageData decoded;
		bool srgb;
		bool valid;
		VkDeviceSize offset;
		TextureType type;
		size_t matIdx;
	};

	std::vector<ResolvedTexture> resolved(futures.size());
	VkDeviceSize totalStagingSize = 0;

	for (size_t i = 0; i < futures.size(); i++) {
		resolved[i].type = static_cast<TextureType>(i % TEXTURES_PER_MATERIAL);
		resolved[i].matIdx = i / TEXTURES_PER_MATERIAL;
		resolved[i].srgb = srgbFlags[i];
		resolved[i].valid = false;

		if (hasFile[i] && futures[i].valid()) {
			resolved[i].decoded = futures[i].get();
			if (resolved[i].decoded.valid) {
				resolved[i].valid = true;
				resolved[i].offset = totalStagingSize;
				totalStagingSize += resolved[i].decoded.imageSize;
			}
		}
	}

	// ========== Create one shared staging buffer ==========
	sharedStagingBuffer_ = VK_NULL_HANDLE;
	sharedStagingMemory_ = VK_NULL_HANDLE;

	if (totalStagingSize > 0) {
		device_.createBuffer(totalStagingSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sharedStagingBuffer_, sharedStagingMemory_);

		// Map once, memcpy all textures
		void* mapped;
		vkMapMemory(device_.device(), sharedStagingMemory_, 0, totalStagingSize, 0, &mapped);

		for (auto& r : resolved) {
			if (r.valid) {
				memcpy(static_cast<char*>(mapped) + r.offset,
					r.decoded.pixels, static_cast<size_t>(r.decoded.imageSize));
			}
		}

		vkUnmapMemory(device_.device(), sharedStagingMemory_);
	}

	// Free all decoded pixels
	for (auto& r : resolved) {
		if (r.decoded.pixels) {
			stbi_image_free(r.decoded.pixels);
			r.decoded.pixels = nullptr;
		}
	}

	// ========== Create SpellTexture objects ==========
	for (size_t i = 0; i < resolved.size(); i++) {
		auto& r = resolved[i];

		if (r.valid) {
			try {
				textures_.push_back(std::make_unique<SpellTexture>(
					device_, r.decoded, r.srgb, true,
					sharedStagingBuffer_, r.offset));
				std::cout << "[Spell] Loaded material[" << r.matIdx << "] "
					<< (r.type == Diffuse ? "diffuse" :
						r.type == Normal ? "normal" :
						r.type == Metallic ? "metallic" : "roughness")
					<< ": " << r.decoded.sourcePath << std::endl;
				continue;
			} catch (const std::exception& e) {
				std::cerr << "[Spell] Failed to create texture from decoded data: " << e.what() << std::endl;
			}
		}

		// Fallback path
		switch (r.type) {
		case Diffuse:
			std::cout << "[Spell] Material[" << r.matIdx << "] has no diffuse texture, using white fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, true, true));
			break;
		case Normal:
			std::cout << "[Spell] Material[" << r.matIdx << "] has no normal texture, using default normal" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true));
			break;
		case Metallic:
			std::cout << "[Spell] Material[" << r.matIdx << "] has no metallic texture, using black fallback" << std::endl;
			textures_.push_back(std::make_unique<SpellTexture>(device_, false, true, 0, 0, 0, 255));
			break;
		case Roughness:
			std::cout << "[Spell] Material[" << r.matIdx << "] has no roughness texture, using mid-gray fallback" << std::endl;
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

	// Clean up all staging buffers (individual ones for fallback textures)
	for (auto& tex : textures_) {
		tex->finalizeStagingCleanup();
	}

	// Destroy shared staging buffer
	if (sharedStagingBuffer_ != VK_NULL_HANDLE) {
		vkDestroyBuffer(device_.device(), sharedStagingBuffer_, nullptr);
		sharedStagingBuffer_ = VK_NULL_HANDLE;
	}
	if (sharedStagingMemory_ != VK_NULL_HANDLE) {
		vkFreeMemory(device_.device(), sharedStagingMemory_, nullptr);
		sharedStagingMemory_ = VK_NULL_HANDLE;
	}

	std::cout << "[Spell] Batched texture upload: " << textures_.size() << " textures in 1 submit" << std::endl;
}

void SpellResourceManager::scanAvailableFiles() {
	availableModels_.clear();
	availableTextures_.clear();

	auto supportedExts = ModelLoaderFactory::allSupportedExtensions();

	if (std::filesystem::exists("assets")) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
			if (!entry.is_regular_file()) continue;
			std::string ext = entry.path().extension().string();
			std::string extLower = ext;
			std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

			if (std::find(supportedExts.begin(), supportedExts.end(), extLower) != supportedExts.end()) {
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
