#pragma once

#include "IModelLoader.h"

namespace Spell {

class ObjModelLoader : public IModelLoader {
public:
	ModelLoadResult load(const std::string& filepath) override;
	std::vector<MaterialInfo> preParseTexturePaths(const std::string& filepath) override;
	std::vector<std::string> supportedExtensions() const override {
		return { ".obj" };
	}
};

} // namespace Spell
