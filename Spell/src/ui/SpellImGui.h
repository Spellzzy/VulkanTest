#pragma once

#include "core/SpellDevice.h"
#include "core/SpellWindow.h"

namespace Spell {

class SpellImGui {
public:
	SpellImGui(SpellWindow& window, SpellDevice& device, VkRenderPass renderPass, uint32_t imageCount);
	~SpellImGui();

	SpellImGui(const SpellImGui&) = delete;
	SpellImGui& operator=(const SpellImGui&) = delete;

	void newFrame();
	void render(VkCommandBuffer commandBuffer);

private:
	SpellDevice& device_;
};

} // namespace Spell
