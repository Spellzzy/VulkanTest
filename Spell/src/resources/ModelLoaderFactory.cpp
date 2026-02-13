#include "ModelLoaderFactory.h"
#include "ObjModelLoader.h"
#include "GltfModelLoader.h"
#include "FbxModelLoader.h"
#include <algorithm>
#include <stdexcept>
#include <filesystem>

namespace Spell {

std::unique_ptr<IModelLoader> ModelLoaderFactory::createLoader(const std::string& filepath) {
	std::string ext = std::filesystem::path(filepath).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".obj") {
		return std::make_unique<ObjModelLoader>();
	}
	if (ext == ".gltf" || ext == ".glb") {
		return std::make_unique<GltfModelLoader>();
	}
	if (ext == ".fbx") {
		return std::make_unique<FbxModelLoader>();
	}

	throw std::runtime_error("[Spell] Unsupported model format: " + ext);
}

std::vector<std::string> ModelLoaderFactory::allSupportedExtensions() {
	return { ".obj", ".gltf", ".glb", ".fbx" };
}

} // namespace Spell
