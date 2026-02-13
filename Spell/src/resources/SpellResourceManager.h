#pragma once

#include "SpellModel.h"
#include "SpellTexture.h"

#include <string>
#include <vector>
#include <memory>

namespace Spell {

class SpellResourceManager {
public:
	SpellResourceManager(SpellDevice& device);

	void scanAvailableFiles();

	const std::string& modelPath() const { return modelPath_; }
	const std::string& texturePath() const { return texturePath_; }
	void setModelPath(const std::string& path) { modelPath_ = path; }
	void setTexturePath(const std::string& path) { texturePath_ = path; }

	const std::vector<std::string>& availableModels() const { return availableModels_; }
	const std::vector<std::string>& availableTextures() const { return availableTextures_; }

	SpellModel* model() const { return model_.get(); }
	SpellTexture* texture() const { return texture_.get(); }

	void loadInitialResources();
	void reloadResources();

private:
	SpellDevice& device_;

	std::string modelPath_{ "models/viking_room.obj" };
	std::string texturePath_{ "textures/viking_room.png" };
	std::vector<std::string> availableModels_;
	std::vector<std::string> availableTextures_;

	std::unique_ptr<SpellModel> model_;
	std::unique_ptr<SpellTexture> texture_;
};

} // namespace Spell
