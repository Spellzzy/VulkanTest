#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormalW;
layout(location = 3) in vec3 fragPositionW;
layout(location = 4) flat in int fragMaterialIndex;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camPos;
} ubo;

layout(push_constant) uniform LightPushConstantData {
	vec3 color;
	vec3 position;
} light;

void main() {
	vec3 N = fragNormalW;
	float nLen = length(N);
	if (nLen < 0.0001) {
		vec3 fdx = dFdx(fragPositionW);
		vec3 fdy = dFdy(fragPositionW);
		N = normalize(cross(fdx, fdy));
	} else {
		N = N / nLen;
	}

	vec3 L = normalize(light.position - fragPositionW);
	float diff = max(dot(N, L), 0.0) * 0.7 + 0.3;
	outColor = vec4(vec3(diff), 1.0);
}
