#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "core/SpellDevice.h"

#include <vector>
#include <array>
#include <string>

#include "robin_hood.h"

namespace Spell {

struct MaterialInfo {
	std::string diffuseTexturePath;
	std::string normalTexturePath;
	std::string metallicTexturePath;
	std::string roughnessTexturePath;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
	int materialIndex;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(Vertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDesc{};
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

		attributeDesc[4].binding = 0;
		attributeDesc[4].location = 4;
		attributeDesc[4].format = VK_FORMAT_R32_SINT;
		attributeDesc[4].offset = offsetof(Vertex, materialIndex);

		return attributeDesc;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord
			&& normal == other.normal && materialIndex == other.materialIndex;
	}
};

} // namespace Spell

namespace std {
	template<> struct hash<Spell::Vertex> {
		size_t operator()(Spell::Vertex const& vertex) const {
			size_t seed = 0;
			auto hashCombine = [&seed](size_t h) {
				seed ^= h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			};
			hashCombine(hash<glm::vec3>()(vertex.pos));
			hashCombine(hash<glm::vec2>()(vertex.texCoord));
			hashCombine(hash<glm::vec3>()(vertex.normal));
			hashCombine(hash<int>()(vertex.materialIndex));
			return seed;
		}
	};
}

namespace Spell {

struct ModelLoadResult;

class SpellModel {
public:
	SpellModel(SpellDevice& device, ModelLoadResult&& data);
	SpellModel(SpellDevice& device, const std::string& modelPath);
	~SpellModel();

	SpellModel(const SpellModel&) = delete;
	SpellModel& operator=(const SpellModel&) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

	uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices_.size()); }
	uint32_t getIndexCount() const { return static_cast<uint32_t>(indices_.size()); }
	const std::vector<MaterialInfo>& getMaterials() const { return materials_; }

private:
	void createVertexBuffer();
	void createIndexBuffer();

	SpellDevice& device_;

	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	std::vector<MaterialInfo> materials_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;
	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;
};

} // namespace Spell
