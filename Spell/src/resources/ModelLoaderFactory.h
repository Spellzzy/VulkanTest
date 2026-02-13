#pragma once

#include "IModelLoader.h"
#include <memory>
#include <string>
#include <vector>

namespace Spell {

class ModelLoaderFactory {
public:
	static std::unique_ptr<IModelLoader> createLoader(const std::string& filepath);
	static std::vector<std::string> allSupportedExtensions();
};

} // namespace Spell
