#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "GltfModelLoader.h"
#include "robin_hood.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cstring>

namespace Spell {

static glm::vec3 toVec3(const float* p) { return { p[0], p[1], p[2] }; }
static glm::vec2 toVec2(const float* p) { return { p[0], p[1] }; }

static const float* accessorReadFloat(const cgltf_accessor* acc, cgltf_size index, float* out, cgltf_size count) {
	if (!acc || index >= acc->count) return nullptr;
	if (cgltf_accessor_read_float(acc, index, out, count)) return out;
	return nullptr;
}

std::string GltfModelLoader::resolveTextureUri(const cgltf_texture* texture, const std::string& baseDir) {
	if (!texture || !texture->image) return "";
	if (texture->image->uri) {
		std::string uri = texture->image->uri;
		if (uri.find("://") != std::string::npos || uri.substr(0, 5) == "data:") {
			return "";
		}
		return baseDir + "/" + uri;
	}
	return "";
}

ModelLoadResult GltfModelLoader::load(const std::string& filepath) {
	ModelLoadResult result;

	cgltf_options options{};
	cgltf_data* data = nullptr;

	cgltf_result parseResult = cgltf_parse_file(&options, filepath.c_str(), &data);
	if (parseResult != cgltf_result_success) {
		throw std::runtime_error("[Spell] glTF: Failed to parse file: " + filepath);
	}

	cgltf_result loadResult = cgltf_load_buffers(&options, data, filepath.c_str());
	if (loadResult != cgltf_result_success) {
		cgltf_free(data);
		throw std::runtime_error("[Spell] glTF: Failed to load buffers: " + filepath);
	}

	std::string baseDir = std::filesystem::path(filepath).parent_path().string();
	if (baseDir.empty()) baseDir = ".";

	// Extract materials
	for (cgltf_size i = 0; i < data->materials_count; i++) {
		const cgltf_material& mat = data->materials[i];
		MaterialInfo info{};

		if (mat.has_pbr_metallic_roughness) {
			const auto& pbr = mat.pbr_metallic_roughness;
			if (pbr.base_color_texture.texture) {
				info.diffuseTexturePath = resolveTextureUri(pbr.base_color_texture.texture, baseDir);
			}
			if (pbr.metallic_roughness_texture.texture) {
				std::string mrPath = resolveTextureUri(pbr.metallic_roughness_texture.texture, baseDir);
				info.metallicTexturePath = mrPath;
				info.roughnessTexturePath = mrPath;
			}
		}
		if (mat.normal_texture.texture) {
			info.normalTexturePath = resolveTextureUri(mat.normal_texture.texture, baseDir);
		}

		result.materials.push_back(info);
	}

	std::cout << "[Spell] glTF: Loaded " << result.materials.size() << " material(s) from " << filepath << std::endl;

	// Process scene nodes
	if (data->scene) {
		for (cgltf_size i = 0; i < data->scene->nodes_count; i++) {
			processNode(data, data->scene->nodes[i], result, baseDir);
		}
	} else if (data->scenes_count > 0) {
		for (cgltf_size i = 0; i < data->scenes[0].nodes_count; i++) {
			processNode(data, data->scenes[0].nodes[i], result, baseDir);
		}
	} else {
		// No scene, process all meshes directly
		for (cgltf_size i = 0; i < data->meshes_count; i++) {
			processMesh(data, &data->meshes[i], result, baseDir);
		}
	}

	cgltf_free(data);

	std::cout << "[Spell] glTF: " << result.vertices.size() << " vertices, "
		<< result.indices.size() << " indices" << std::endl;

	return result;
}

void GltfModelLoader::processNode(const cgltf_data* data, const cgltf_node* node,
	ModelLoadResult& result, const std::string& baseDir) {
	if (node->mesh) {
		processMesh(data, node->mesh, result, baseDir);
	}
	for (cgltf_size i = 0; i < node->children_count; i++) {
		processNode(data, node->children[i], result, baseDir);
	}
}

void GltfModelLoader::processMesh(const cgltf_data* data, const cgltf_mesh* mesh,
	ModelLoadResult& result, const std::string& baseDir) {

	for (cgltf_size p = 0; p < mesh->primitives_count; p++) {
		const cgltf_primitive& prim = mesh->primitives[p];

		if (prim.type != cgltf_primitive_type_triangles) continue;

		// Find material index
		int materialIndex = -1;
		if (prim.material) {
			for (cgltf_size mi = 0; mi < data->materials_count; mi++) {
				if (&data->materials[mi] == prim.material) {
					materialIndex = static_cast<int>(mi);
					break;
				}
			}
		}

		// Find accessors
		const cgltf_accessor* posAcc = nullptr;
		const cgltf_accessor* normalAcc = nullptr;
		const cgltf_accessor* texCoordAcc = nullptr;

		for (cgltf_size a = 0; a < prim.attributes_count; a++) {
			if (prim.attributes[a].type == cgltf_attribute_type_position) {
				posAcc = prim.attributes[a].data;
			} else if (prim.attributes[a].type == cgltf_attribute_type_normal) {
				normalAcc = prim.attributes[a].data;
			} else if (prim.attributes[a].type == cgltf_attribute_type_texcoord && prim.attributes[a].index == 0) {
				texCoordAcc = prim.attributes[a].data;
			}
		}

		if (!posAcc) continue;

		uint32_t baseVertex = static_cast<uint32_t>(result.vertices.size());

		// Read vertices
		for (cgltf_size vi = 0; vi < posAcc->count; vi++) {
			Vertex vertex{};
			float buf[3];

			if (accessorReadFloat(posAcc, vi, buf, 3)) {
				vertex.pos = toVec3(buf);
			}
			if (accessorReadFloat(normalAcc, vi, buf, 3)) {
				vertex.normal = toVec3(buf);
			} else {
				vertex.normal = { 0.0f, 0.0f, 1.0f };
			}
			float uv[2] = { 0.0f, 0.0f };
			if (accessorReadFloat(texCoordAcc, vi, uv, 2)) {
				vertex.texCoord = toVec2(uv);
			}

			vertex.color = { 1.0f, 1.0f, 1.0f };
			vertex.materialIndex = materialIndex;

			result.vertices.push_back(vertex);
		}

		// Read indices
		if (prim.indices) {
			for (cgltf_size ii = 0; ii < prim.indices->count; ii++) {
				uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii));
				result.indices.push_back(baseVertex + idx);
			}
		} else {
			// Non-indexed: generate sequential indices
			for (cgltf_size vi = 0; vi < posAcc->count; vi++) {
				result.indices.push_back(baseVertex + static_cast<uint32_t>(vi));
			}
		}
	}
}

std::vector<MaterialInfo> GltfModelLoader::preParseTexturePaths(const std::string& filepath) {
	cgltf_options options{};
	cgltf_data* data = nullptr;

	cgltf_result parseResult = cgltf_parse_file(&options, filepath.c_str(), &data);
	if (parseResult != cgltf_result_success) {
		return {};
	}

	std::string baseDir = std::filesystem::path(filepath).parent_path().string();
	if (baseDir.empty()) baseDir = ".";

	std::vector<MaterialInfo> result;
	for (cgltf_size i = 0; i < data->materials_count; i++) {
		const cgltf_material& mat = data->materials[i];
		MaterialInfo info{};

		if (mat.has_pbr_metallic_roughness) {
			const auto& pbr = mat.pbr_metallic_roughness;
			if (pbr.base_color_texture.texture) {
				info.diffuseTexturePath = resolveTextureUri(pbr.base_color_texture.texture, baseDir);
			}
			if (pbr.metallic_roughness_texture.texture) {
				std::string mrPath = resolveTextureUri(pbr.metallic_roughness_texture.texture, baseDir);
				info.metallicTexturePath = mrPath;
				info.roughnessTexturePath = mrPath;
			}
		}
		if (mat.normal_texture.texture) {
			info.normalTexturePath = resolveTextureUri(mat.normal_texture.texture, baseDir);
		}

		result.push_back(info);
	}

	cgltf_free(data);

	std::cout << "[Spell] glTF: Pre-parsed " << result.size() << " material(s) for texture paths" << std::endl;
	return result;
}

} // namespace Spell
