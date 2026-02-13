#include "SpellInspector.h"
#include "renderer/SpellTypes.h"

#include <imgui.h>

namespace Spell {

bool SpellInspector::draw(SpellResourceManager& resources, LightPushConstantData& light) {
	bool needReload = false;

	ImGui::Begin("Inspector");

	ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Separator();

	ImGui::Text("Light");
	ImGui::ColorEdit3("Color", lightColor_);
	ImGui::DragFloat("Intensity", &lightIntensity_, 0.1f, 0.0f, 100.0f);
	light.color = glm::vec3(lightColor_[0], lightColor_[1], lightColor_[2]) * lightIntensity_;
	ImGui::DragFloat3("Position", &light.position.x, 0.1f, -10.0f, 10.0f);

	ImGui::Separator();
	ImGui::Text("Resources");

	auto& availableModels = resources.availableModels();
	auto& availableTextures = resources.availableTextures();

	// Model selector
	auto modelGetter = [](void* data, int idx, const char** out_text) -> bool {
		auto& models = *static_cast<const std::vector<std::string>*>(data);
		if (idx < 0 || idx >= static_cast<int>(models.size())) return false;
		*out_text = models[idx].c_str();
		return true;
	};
	ImGui::Combo("Model", &selectedModelIdx_, modelGetter,
		const_cast<void*>(static_cast<const void*>(&availableModels)),
		static_cast<int>(availableModels.size()));

	if (resources.model()) {
		ImGui::Text("  Vertices: %u  Indices: %u",
			resources.model()->getVertexCount(), resources.model()->getIndexCount());
		ImGui::Text("  Materials: %u  Textures: %u",
			static_cast<uint32_t>(resources.model()->getMaterials().size()),
			resources.textureCount());
	}

	// Fallback texture selector (used when model has no materials)
	bool hasMaterials = resources.model() && !resources.model()->getMaterials().empty();
	auto texGetter = [](void* data, int idx, const char** out_text) -> bool {
		auto& textures = *static_cast<const std::vector<std::string>*>(data);
		if (idx < 0 || idx >= static_cast<int>(textures.size())) return false;
		*out_text = textures[idx].c_str();
		return true;
	};
	if (hasMaterials) {
		ImGui::BeginDisabled();
	}
	ImGui::Combo("Fallback Tex", &selectedTextureIdx_, texGetter,
		const_cast<void*>(static_cast<const void*>(&availableTextures)),
		static_cast<int>(availableTextures.size()));
	if (hasMaterials) {
		ImGui::EndDisabled();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(model uses .mtl textures)");
	}

	ImGui::Spacing();

	bool modelChanged = (!availableModels.empty() &&
		selectedModelIdx_ < static_cast<int>(availableModels.size()) &&
		availableModels[selectedModelIdx_] != resources.modelPath());
	bool textureChanged = (!availableTextures.empty() &&
		selectedTextureIdx_ < static_cast<int>(availableTextures.size()) &&
		availableTextures[selectedTextureIdx_] != resources.texturePath());

	if (modelChanged || textureChanged) {
		if (ImGui::Button("Apply Changes")) {
			if (modelChanged) resources.setModelPath(availableModels[selectedModelIdx_]);
			if (textureChanged) resources.setTexturePath(availableTextures[selectedTextureIdx_]);
			needReload = true;
		}
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "(pending changes)");
	}

	if (ImGui::Button("Force Reload")) {
		needReload = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh File List")) {
		resources.scanAvailableFiles();
		syncSelection(resources);
	}

	// Material texture info
	if (resources.model() && !resources.model()->getMaterials().empty()) {
		ImGui::Separator();
		ImGui::Text("Material Textures (auto-loaded from .mtl):");
		const auto& mats = resources.model()->getMaterials();
		for (size_t i = 0; i < mats.size(); i++) {
			ImGui::Text("  [%zu] diffuse: %s", i,
				mats[i].diffuseTexturePath.empty() ? "(none)" : mats[i].diffuseTexturePath.c_str());
			ImGui::Text("       normal: %s",
				mats[i].normalTexturePath.empty() ? "(none)" : mats[i].normalTexturePath.c_str());
			ImGui::Text("       metallic: %s",
				mats[i].metallicTexturePath.empty() ? "(none)" : mats[i].metallicTexturePath.c_str());
			ImGui::Text("       roughness: %s",
				mats[i].roughnessTexturePath.empty() ? "(none)" : mats[i].roughnessTexturePath.c_str());
		}
	}

	ImGui::End();

	return needReload;
}

void SpellInspector::syncSelection(const SpellResourceManager& resources) {
	auto& models = resources.availableModels();
	auto& textures = resources.availableTextures();

	for (int i = 0; i < static_cast<int>(models.size()); i++) {
		if (models[i] == resources.modelPath()) { selectedModelIdx_ = i; break; }
	}
	for (int i = 0; i < static_cast<int>(textures.size()); i++) {
		if (textures[i] == resources.texturePath()) { selectedTextureIdx_ = i; break; }
	}
}

} // namespace Spell
