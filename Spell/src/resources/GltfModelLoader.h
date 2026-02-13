#pragma once

#include "IModelLoader.h"

namespace Spell {

class GltfModelLoader : public IModelLoader {
public:
	ModelLoadResult load(const std::string& filepath) override;
	std::vector<MaterialInfo> preParseTexturePaths(const std::string& filepath) override;
	std::vector<std::string> supportedExtensions() const override {
		return { ".gltf", ".glb" };
	}

private:
	void processNode(const struct cgltf_data* data, const struct cgltf_node* node,
		ModelLoadResult& result, const std::string& baseDir);
	void processMesh(const struct cgltf_data* data, const struct cgltf_mesh* mesh,
		ModelLoadResult& result, const std::string& baseDir);
	std::string resolveTextureUri(const struct cgltf_texture* texture, const std::string& baseDir);
};

} // namespace Spell
