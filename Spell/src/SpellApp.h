#pragma once

#include "core/SpellWindow.h"
#include "core/SpellDevice.h"
#include "renderer/SpellRenderer.h"
#include "renderer/SpellPipeline.h"
#include "renderer/SpellTypes.h"
#include "resources/SpellResourceManager.h"
#include "ui/SpellImGui.h"
#include "ui/SpellInspector.h"

#include <memory>
#include <vector>

namespace Spell {

class SpellApp {
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	SpellApp();
	~SpellApp();

	SpellApp(const SpellApp&) = delete;
	SpellApp& operator=(const SpellApp&) = delete;

	void run();

private:
	void createPipelineLayout();
	void createPipeline();
	void createDescriptorSetLayout();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void updateUniformBuffer(int frameIndex);
	void renderFrame();
	void drawImGuiPanels();
	void rebuildDescriptors();

	SpellWindow window_{ WIDTH, HEIGHT, "Spell Engine" };
	SpellDevice device_{ window_ };
	SpellRenderer renderer_{ window_, device_ };

	std::unique_ptr<SpellPipeline> pipeline_;
	VkPipelineLayout pipelineLayout_;
	VkDescriptorSetLayout descriptorSetLayout_;
	VkDescriptorPool descriptorPool_;
	std::vector<VkDescriptorSet> descriptorSets_;

	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;

	std::unique_ptr<SpellImGui> imgui_;

	// Pipeline Statistics Query
	VkQueryPool statsQueryPool_ = VK_NULL_HANDLE;
	static constexpr uint32_t STATS_QUERY_COUNT = 5; // number of pipeline statistic bits
	bool statsQueryReady_ = false;

	// Subsystems
	SpellResourceManager resources_{ device_ };
	SpellInspector inspector_;

	bool needReload_{ false };
	bool convertYUp_{ false };
	LightPushConstantData lightData_{ glm::vec3(23.47f, 21.31f, 20.79f), glm::vec3(2.0f, 2.0f, 2.0f) };
	RenderStats renderStats_{};
};

} // namespace Spell
