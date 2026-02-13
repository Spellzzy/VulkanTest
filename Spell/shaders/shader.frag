#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 camPos;
} ubo;

layout(binding = 1) uniform sampler2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormalW;
layout(location = 3) in vec3 fragPositionW;
layout(location = 4) flat in int fragMaterialIndex;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LightPushConstantData {
	vec3 color;
	vec3 position;
} light;

const float PI = 3.14159265359;

// Normal Distribution Function - GGX/Trowbridge-Reitz
float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return a2 / max(denom, 0.0000001);
}

// Geometry Function - Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	return NdotV / (NdotV * (1.0 - k) + k);
}

// Geometry Function - Smith's method
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// Fresnel - Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
	// Texture index calculation:
	// Layout: [fallback_diff(0), fallback_norm(1), fallback_metal(2), fallback_rough(3),
	//          mat0_diff(4), mat0_norm(5), mat0_metal(6), mat0_rough(7), ...]
	// Each material occupies 4 slots starting at offset 4
	int diffuseIdx   = fragMaterialIndex >= 0 ? (fragMaterialIndex + 1) * 4     : 0;
	int normalIdx    = fragMaterialIndex >= 0 ? (fragMaterialIndex + 1) * 4 + 1 : 1;
	int metallicIdx  = fragMaterialIndex >= 0 ? (fragMaterialIndex + 1) * 4 + 2 : 2;
	int roughnessIdx = fragMaterialIndex >= 0 ? (fragMaterialIndex + 1) * 4 + 3 : 3;

	// Sample textures
	vec3 albedo = pow(texture(textures[nonuniformEXT(diffuseIdx)], fragTexCoord).rgb, vec3(2.2));
	float metallic = texture(textures[nonuniformEXT(metallicIdx)], fragTexCoord).r;
	float roughness = texture(textures[nonuniformEXT(roughnessIdx)], fragTexCoord).r;
	roughness = max(roughness, 0.04);

	// Build TBN matrix from screen-space derivatives
	vec3 N = fragNormalW;
	float nLen = length(N);
	if (nLen < 0.0001) {
		// Fallback: derive normal from screen-space derivatives
		vec3 fdx = dFdx(fragPositionW);
		vec3 fdy = dFdy(fragPositionW);
		N = normalize(cross(fdx, fdy));
	} else {
		N = N / nLen;
	}

	vec3 Q1 = dFdx(fragPositionW);
	vec3 Q2 = dFdy(fragPositionW);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);

	float det = st1.s * st2.t - st2.s * st1.t;
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = normalize(cross(N, T));
	if (det < 0.0) {
		T = -T;
	}
	mat3 TBN = mat3(T, B, N);

	// Sample normal map and transform to world space
	vec3 normalMap = texture(textures[nonuniformEXT(normalIdx)], fragTexCoord).rgb;
	normalMap = normalMap * 2.0 - 1.0;
	N = normalize(TBN * normalMap);

	// View and light directions
	vec3 V = normalize(ubo.camPos - fragPositionW);
	vec3 L = normalize(light.position - fragPositionW);
	vec3 H = normalize(V + L);

	// F0 - base reflectivity
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Cook-Torrance BRDF
	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	// Specular
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	vec3 numerator = D * G * F;
	float denominator = 4.0 * NdotV * NdotL + 0.0001;
	vec3 specular = numerator / denominator;

	// Energy conservation
	vec3 kS = F;
	vec3 kD = (1.0 - kS) * (1.0 - metallic);

	// Point light radiance
	float distance = length(light.position - fragPositionW);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = light.color * attenuation;

	// Outgoing radiance
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

	// Ambient (simple approximation, could be IBL later)
	vec3 ambient = vec3(0.03) * albedo;

	vec3 color = ambient + Lo;

	// Reinhard tone mapping
	color = color / (color + vec3(1.0));

	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);
}
