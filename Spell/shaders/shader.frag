#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 fragPosition;
layout(location = 4) flat in int fragMaterialIndex;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LightPushConstantData {
	vec3 color;
	vec3 position;
} light;

void main() {
	// materialIndex mapping:
	// -1 or invalid -> use texture index 0 (fallback)
	// >= 0 -> offset by 1 because index 0 is fallback white texture
	int texIdx = fragMaterialIndex >= 0 ? fragMaterialIndex + 1 : 0;

	vec3 normalDir = normalize(fragNormal);
	vec3 lightDir = normalize(fragPosition - vec4(light.position, 1.0)).xyz;

	float NdotL = max(0, dot(normalDir, lightDir));
	vec3 directionDiffuse = pow(NdotL, 0.5) * light.color;
	outColor = texture(textures[nonuniformEXT(texIdx)], fragTexCoord) * vec4(directionDiffuse, 1.0);
}
