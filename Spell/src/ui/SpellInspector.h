#pragma once

#include "resources/SpellResourceManager.h"
#include "renderer/SpellTypes.h"

namespace Spell {

class SpellInspector {
public:
	// 返回 true 表示资源需要重载
	bool draw(SpellResourceManager& resources, LightPushConstantData& light);

private:
	int selectedModelIdx_{ 0 };
	int selectedTextureIdx_{ 0 };

	void syncSelection(const SpellResourceManager& resources);
};

} // namespace Spell
