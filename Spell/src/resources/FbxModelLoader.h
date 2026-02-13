#pragma once

#include "IModelLoader.h"

namespace Spell {

class FbxModelLoader : public IModelLoader {
public:
	ModelLoadResult load(const std::string& filepath) override;
	std::vector<MaterialInfo> preParseTexturePaths(const std::string& filepath) override;
	std::vector<std::string> supportedExtensions() const override {
		return { ".fbx" };
	}

private:
	std::string resolveTexturePath(const std::string& rawPath, const std::string& baseDir);
};

} // namespace Spell
