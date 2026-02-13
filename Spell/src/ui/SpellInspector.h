#pragma once

#include "resources/SpellResourceManager.h"
#include "renderer/SpellTypes.h"

namespace Spell {

class SpellInspector {
public:
	// Returns true if resources need to be reloaded
	bool draw(SpellResourceManager& resources, LightPushConstantData& light, bool& convertYUp, const RenderStats& stats, RenderMode& renderMode);

private:
	int selectedModelIdx_{ 0 };
	int selectedTextureIdx_{ 0 };
	float lightColor_[3]{ 1.0f, 0.92f, 0.9f };
	float lightIntensity_{ 23.0f };

	void syncSelection(const SpellResourceManager& resources);
};

} // namespace Spell
