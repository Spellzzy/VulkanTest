#pragma once

#include "SpellModel.h"
#include <string>
#include <vector>
#include <memory>

namespace Spell {

struct ModelLoadResult {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<MaterialInfo> materials;
};

class IModelLoader {
public:
	virtual ~IModelLoader() = default;

	virtual ModelLoadResult load(const std::string& filepath) = 0;

	virtual std::vector<MaterialInfo> preParseTexturePaths(const std::string& filepath) {
		return {};
	}

	virtual std::vector<std::string> supportedExtensions() const = 0;
};

} // namespace Spell
