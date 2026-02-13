#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "SpellModel.h"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <filesystem>

namespace Spell {

SpellModel::SpellModel(SpellDevice& device, const std::string& modelPath)
	: device_(device) {
	loadModel(modelPath);
	createVertexBuffer();
	createIndexBuffer();
}

SpellModel::~SpellModel() {
	vkDestroyBuffer(device_.device(), indexBuffer_, nullptr);
	vkFreeMemory(device_.device(), indexBufferMemory_, nullptr);
	vkDestroyBuffer(device_.device(), vertexBuffer_, nullptr);
	vkFreeMemory(device_.device(), vertexBufferMemory_, nullptr);
}

void SpellModel::loadModel(const std::string& filepath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::string mtlBaseDir = std::filesystem::path(filepath).parent_path().string();
	if (mtlBaseDir.empty()) mtlBaseDir = ".";
	mtlBaseDir += "/";

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlBaseDir.c_str())) {
		throw std::runtime_error(warn + err);
	}

	// Build material info list
	materials_.clear();
	for (const auto& mat : materials) {
		MaterialInfo info{};
		if (!mat.diffuse_texname.empty()) {
			info.diffuseTexturePath = mtlBaseDir + mat.diffuse_texname;
		}
		materials_.push_back(info);
	}

	std::cout << "[Spell] Loaded " << materials_.size() << " material(s) from " << filepath << std::endl;
	for (size_t i = 0; i < materials_.size(); i++) {
		std::cout << "  [" << i << "] diffuse: "
			<< (materials_[i].diffuseTexturePath.empty() ? "(none)" : materials_[i].diffuseTexturePath) << std::endl;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		size_t indexOffset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int faceVerts = shape.mesh.num_face_vertices[f];
			int matId = -1;
			if (f < shape.mesh.material_ids.size()) {
				matId = shape.mesh.material_ids[f];
			}
			// materialIndex: -1 means no material, use fallback (index 0 in texture array)
			int materialIndex = (matId >= 0 && matId < static_cast<int>(materials_.size())) ? matId : -1;

			for (int v = 0; v < faceVerts; v++) {
				const auto& index = shape.mesh.indices[indexOffset + v];
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (index.texcoord_index >= 0) {
					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}

				vertex.materialIndex = materialIndex;

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
					vertices_.push_back(vertex);
				}
				indices_.push_back(uniqueVertices[vertex]);
			}
			indexOffset += faceVerts;
		}
	}
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
