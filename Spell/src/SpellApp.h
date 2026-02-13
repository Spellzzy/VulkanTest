#pragma once

#include "core/SpellWindow.h"
#include "core/SpellDevice.h"
#include "renderer/SpellRenderer.h"
#include "renderer/SpellPipeline.h"
#include "resources/SpellModel.h"
#include "resources/SpellTexture.h"

#include <memory>
#include <vector>

namespace Spell {

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct LightPushConstantData {
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec3 position;
};

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

	std::unique_ptr<SpellModel> model_;
	std::unique_ptr<SpellTexture> texture_;

	LightPushConstantData lightData_{};
};

} // namespace Spell
