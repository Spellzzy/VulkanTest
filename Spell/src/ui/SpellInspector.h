#pragma once

#include "resources/SpellResourceManager.h"
#include "renderer/SpellTypes.h"

namespace Spell {

class SpellInspector {
public:
	// Returns true if resources need to be reloaded
	bool draw(SpellResourceManager& resources, LightPushConstantData& light);

private:
	int selectedModelIdx_{ 0 };
	int selectedTextureIdx_{ 0 };

	void syncSelection(const SpellResourceManager& resources);
};

} // namespace Spell
