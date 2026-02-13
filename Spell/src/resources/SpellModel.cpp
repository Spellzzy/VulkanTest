#include "SpellModel.h"
#include "IModelLoader.h"
#include "ModelLoaderFactory.h"
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace Spell {

SpellModel::SpellModel(SpellDevice& device, ModelLoadResult&& data)
	: device_(device) {
	vertices_ = std::move(data.vertices);
	indices_ = std::move(data.indices);
	materials_ = std::move(data.materials);
	createVertexBuffer();
	createIndexBuffer();
}

SpellModel::SpellModel(SpellDevice& device, const std::string& modelPath)
	: device_(device) {
	auto loader = ModelLoaderFactory::createLoader(modelPath);
	auto result = loader->load(modelPath);
	vertices_ = std::move(result.vertices);
	indices_ = std::move(result.indices);
	materials_ = std::move(result.materials);
	createVertexBuffer();
	createIndexBuffer();
}

SpellModel::~SpellModel() {
	vkDestroyBuffer(device_.device(), indexBuffer_, nullptr);
	vkFreeMemory(device_.device(), indexBufferMemory_, nullptr);
	vkDestroyBuffer(device_.device(), vertexBuffer_, nullptr);
	vkFreeMemory(device_.device(), vertexBufferMemory_, nullptr);
}

void SpellModel::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices_.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device_.device(), stagingBufferMemory);

	device_.createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);

	device_.copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);

	vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
	vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
}

void SpellModel::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices_.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device_.device(), stagingBufferMemory);

	device_.createBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);

	device_.copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

	vkDestroyBuffer(device_.device(), stagingBuffer, nullptr);
	vkFreeMemory(device_.device(), stagingBufferMemory, nullptr);
}

void SpellModel::bind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[] = { vertexBuffer_ };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
}

void SpellModel::draw(VkCommandBuffer commandBuffer) {
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
}

} // namespace Spell
