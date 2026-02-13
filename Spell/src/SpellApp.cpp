#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "SpellApp.h"
#include <imgui.h>
#include <chrono>
#include <stdexcept>
#include <array>

namespace Spell {

SpellApp::SpellApp() {
	createDescriptorSetLayout();
	createPipelineLayout();
	createPipeline();

	resources_.loadInitialResources();

	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

	imgui_ = std::make_unique<SpellImGui>(
		window_, device_, renderer_.getSwapChainRenderPass(),
		static_cast<uint32_t>(renderer_.getSwapChainImageCount()));
}

SpellApp::~SpellApp() {
	vkDeviceWaitIdle(device_.device());
	imgui_.reset();

	for (size_t i = 0; i < uniformBuffers_.size(); i++) {
		vkDestroyBuffer(device_.device(), uniformBuffers_[i], nullptr);
		vkFreeMemory(device_.device(), uniformBuffersMemory_[i], nullptr);
	}

	vkDestroyDescriptorPool(device_.device(), descriptorPool_, nullptr);
	vkDestroyDescriptorSetLayout(device_.device(), descriptorSetLayout_, nullptr);
	vkDestroyPipelineLayout(device_.device(), pipelineLayout_, nullptr);
}

void SpellApp::run() {
	while (!window_.shouldClose()) {
		glfwPollEvents();
		renderFrame();
	}
	vkDeviceWaitIdle(device_.device());
}

void SpellApp::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = MAX_BINDLESS_TEXTURES;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	// Binding flags for bindless
	std::array<VkDescriptorBindingFlags, 2> bindingFlags{};
	bindingFlags[0] = 0; // UBO: no special flags
	bindingFlags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		| VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
		| VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
	bindingFlagsInfo.pBindingFlags = bindingFlags.data();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = &bindingFlagsInfo;
	layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device_.device(), &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void SpellApp::createPipelineLayout() {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(LightPushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device_.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void SpellApp::createPipeline() {
	PipelineConfigInfo pipelineConfig{};
	SpellPipeline::defaultPipelineConfigInfo(pipelineConfig, device_.msaaSamples());
	pipelineConfig.renderPass = renderer_.getSwapChainRenderPass();
	pipelineConfig.pipelineLayout = pipelineLayout_;

	pipeline_ = std::make_unique<SpellPipeline>(
		device_, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);
}

void SpellApp::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	size_t imageCount = renderer_.getSwapChainImageCount();

	uniformBuffers_.resize(imageCount);
	uniformBuffersMemory_.resize(imageCount);

	for (size_t i = 0; i < imageCount; i++) {
		device_.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers_[i], uniformBuffersMemory_[i]);
	}
}

void SpellApp::createDescriptorPool() {
	size_t imageCount = renderer_.getSwapChainImageCount();

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(imageCount);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = MAX_BINDLESS_TEXTURES * static_cast<uint32_t>(imageCount);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(imageCount);

	if (vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void SpellApp::createDescriptorSets() {
	size_t imageCount = renderer_.getSwapChainImageCount();
	uint32_t actualTextureCount = resources_.textureCount();
	if (actualTextureCount == 0) actualTextureCount = 1;

	std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout_);

	// Variable descriptor count for the last binding (bindless textures)
	std::vector<uint32_t> variableCounts(imageCount, actualTextureCount);

	VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
	variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
	variableCountInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
	variableCountInfo.pDescriptorCounts = variableCounts.data();

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = &variableCountInfo;
	allocInfo.descriptorPool = descriptorPool_;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets_.resize(imageCount);
	if (vkAllocateDescriptorSets(device_.device(), &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < imageCount; i++) {
		// UBO write
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers_[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet uboWrite{};
		uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboWrite.dstSet = descriptorSets_[i];
		uboWrite.dstBinding = 0;
		uboWrite.dstArrayElement = 0;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWrite.descriptorCount = 1;
		uboWrite.pBufferInfo = &bufferInfo;

		// Bindless texture array write
		std::vector<VkDescriptorImageInfo> imageInfos(actualTextureCount);
		for (uint32_t t = 0; t < actualTextureCount; t++) {
			imageInfos[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[t].imageView = resources_.textures()[t]->getImageView();
			imageInfos[t].sampler = resources_.textures()[t]->getSampler();
		}

		VkWriteDescriptorSet textureWrite{};
		textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textureWrite.dstSet = descriptorSets_[i];
		textureWrite.dstBinding = 1;
		textureWrite.dstArrayElement = 0;
		textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.descriptorCount = actualTextureCount;
		textureWrite.pImageInfo = imageInfos.data();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = { uboWrite, textureWrite };
		vkUpdateDescriptorSets(device_.device(), static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);
	}
}

void SpellApp::updateUniformBuffer(int frameIndex) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	glm::mat4 modelMat = glm::rotate(
		glm::mat4(1.0f),
		time * glm::radians(10.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	if (convertYUp_) {
		glm::mat4 coordFix = glm::rotate(
			glm::mat4(1.0f),
			glm::radians(90.0f),
			glm::vec3(1.0f, 0.0f, 0.0f));
		modelMat = modelMat * coordFix;
	}

	ubo.model = modelMat;

	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.camPos = glm::vec3(2.0f, 2.0f, 2.0f);

	auto extent = renderer_.getSwapChainExtent();
	ubo.proj = glm::perspective(
		glm::radians(45.0f),
		extent.width / static_cast<float>(extent.height),
		0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device_.device(), uniformBuffersMemory_[frameIndex], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device_.device(), uniformBuffersMemory_[frameIndex]);
}

void SpellApp::renderFrame() {
	if (needReload_) {
		resources_.reloadResources();
		rebuildDescriptors();
		needReload_ = false;
	}

	auto commandBuffer = renderer_.beginFrame();
	if (commandBuffer == nullptr) return;

	int frameIndex = renderer_.getFrameIndex();
	updateUniformBuffer(frameIndex);

	renderer_.beginRenderPass(commandBuffer);

	vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof(LightPushConstantData), &lightData_);

	pipeline_->bind(commandBuffer);
	resources_.model()->bind(commandBuffer);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_, 0, 1, &descriptorSets_[frameIndex], 0, nullptr);

	resources_.model()->draw(commandBuffer);

	// Collect render stats
	renderStats_ = {};
	renderStats_.drawCalls = 1;
	renderStats_.vertices = resources_.model()->getVertexCount();
	renderStats_.indices = resources_.model()->getIndexCount();
	renderStats_.triangles = renderStats_.indices / 3;
	renderStats_.textureCount = resources_.textureCount();
	renderStats_.materialCount = static_cast<uint32_t>(resources_.model()->getMaterials().size());
	renderStats_.fps = ImGui::GetIO().Framerate;
	renderStats_.frameTimeMs = 1000.0f / renderStats_.fps;

	imgui_->newFrame();
	drawImGuiPanels();
	imgui_->render(commandBuffer);

	renderer_.endRenderPass(commandBuffer);
	renderer_.endFrame();
}

void SpellApp::drawImGuiPanels() {
	if (inspector_.draw(resources_, lightData_, convertYUp_, renderStats_)) {
		needReload_ = true;
	}
}

void SpellApp::rebuildDescriptors() {
	vkDestroyDescriptorPool(device_.device(), descriptorPool_, nullptr);
	descriptorSets_.clear();

	createDescriptorPool();
	createDescriptorSets();
}

} // namespace Spell
