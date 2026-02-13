#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

namespace Spell {

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 camPos;
};

struct LightPushConstantData {
	alignas(16) glm::vec3 color;
	alignas(16) glm::vec3 position;
};

struct RenderStats {
	uint32_t drawCalls = 0;
	uint32_t vertices = 0;
	uint32_t indices = 0;
	uint32_t triangles = 0;
	uint32_t textureCount = 0;
	uint32_t materialCount = 0;
	float frameTimeMs = 0.0f;
	float fps = 0.0f;
};

} // namespace Spell
