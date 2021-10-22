#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LightPushConstantData {
	vec3 color;
	vec3 position;
} light;

void main() {
	//outColor = vec4(fragColor * texture(texSampler, fragTexCoord * 1.0).rgb , 1.0) ;
	//vec3 normal = normalize(fragNormal);
	outColor = texture(texSampler, fragTexCoord) * vec4(light.color, 1.0);
}