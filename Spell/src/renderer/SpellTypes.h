#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

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

} // namespace Spell
