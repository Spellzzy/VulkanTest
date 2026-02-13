#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "core/SpellDevice.h"

#include <vector>
#include <array>
#include <unordered_map>

namespace Spell {

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(Vertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDesc{};
		attributeDesc[0].binding = 0;
		attributeDesc[0].location = 0;
		attributeDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[0].offset = offsetof(Vertex, pos);

		attributeDesc[1].binding = 0;
		attributeDesc[1].location = 1;
		attributeDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[1].offset = offsetof(Vertex, color);

		attributeDesc[2].binding = 0;
		attributeDesc[2].location = 2;
		attributeDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDesc[2].offset = offsetof(Vertex, texCoord);

		attributeDesc[3].binding = 0;
		attributeDesc[3].location = 3;
		attributeDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[3].offset = offsetof(Vertex, normal);

		return attributeDesc;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

} // namespace Spell

namespace std {
	template<> struct hash<Spell::Vertex> {
		size_t operator()(Spell::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

namespace Spell {

class SpellModel {
public:
	SpellModel(SpellDevice& device, const std::string& modelPath);
	~SpellModel();

	SpellModel(const SpellModel&) = delete;
	SpellModel& operator=(const SpellModel&) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

	uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices_.size()); }
	uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }

private:
	void loadModel(const std::string& filepath);
	void createVertexBuffer();
	void createIndexBuffer();

	SpellDevice& device_;

	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;
	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;
};

} // namespace Spell
