#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "SpellApp.h"
#include <imgui.h>
#include <chrono>
#include <stdexcept>
#include <array>
#include <iostream>

namespace Spell {

SpellApp::SpellApp() {
	createDescriptorSetLayout();
	createPipelineLayout();
	createPipeline();

	model_ = std::make_unique<SpellModel>(device_, "models/viking_room.obj");
	texture_ = std::make_unique<SpellTexture>(device_, "textures/viking_room.png");

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
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
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
	poolSizes[1].descriptorCount = static_cast<uint32_t>(imageCount);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(imageCount);

	if (vkCreateDescriptorPool(device_.device(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void SpellApp::createDescriptorSets() {
	size_t imageCount = renderer_.getSwapChainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout_);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool_;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets_.resize(imageCount);
	if (vkAllocateDescriptorSets(device_.device(), &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < imageCount; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers_[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture_->getImageView();
		imageInfo.sampler = texture_->getSampler();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets_[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets_[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device_.device(), static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);
	}
}

void SpellApp::updateUniformBuffer(int frameIndex) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(
		(glm::mat4(1.0f) * sin(time) * glm::mat4(0.5) + glm::mat4(0.5)),
		time * glm::radians(10.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

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
	auto commandBuffer = renderer_.beginFrame();
	if (commandBuffer == nullptr) return;

	int frameIndex = renderer_.getFrameIndex();
	updateUniformBuffer(frameIndex);

	renderer_.beginRenderPass(commandBuffer);

	vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof(LightPushConstantData), &lightData_);

	pipeline_->bind(commandBuffer);
	model_->bind(commandBuffer);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_, 0, 1, &descriptorSets_[frameIndex], 0, nullptr);

	model_->draw(commandBuffer);

	// ImGui 绘制（在同一个 RenderPass 内，场景之后）
	imgui_->newFrame();
	drawImGuiPanels();
	imgui_->render(commandBuffer);

	renderer_.endRenderPass(commandBuffer);
	renderer_.endFrame();
}

void SpellApp::drawImGuiPanels() {
	ImGui::Begin("Spell Engine");

	ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Separator();

	ImGui::Text("Light");
	ImGui::ColorEdit3("Color", &lightData_.color.x);
	ImGui::DragFloat3("Light Position", &lightData_.position.x, 0.1f, -10.0f, 10.0f);

	ImGui::End();
}

} // namespace Spell
