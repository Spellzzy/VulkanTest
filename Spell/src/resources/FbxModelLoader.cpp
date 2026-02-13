#include <ufbx.h>

#include "FbxModelLoader.h"
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace Spell {

std::string FbxModelLoader::resolveTexturePath(const std::string& rawPath, const std::string& baseDir) {
	if (rawPath.empty()) return "";

	// If file exists as-is, use it
	if (std::filesystem::exists(rawPath)) return rawPath;

	// Extract just the filename and look in the base directory
	std::string filename = std::filesystem::path(rawPath).filename().string();
	std::string resolved = baseDir + "/" + filename;
	if (std::filesystem::exists(resolved)) return resolved;

	// Try common subdirectories
	for (const char* subdir : { "textures", "Textures", "tex", "maps" }) {
		std::string sub = baseDir + "/" + subdir + "/" + filename;
		if (std::filesystem::exists(sub)) return sub;
	}

	// Return the baseDir + filename as best guess
	return resolved;
}

static std::string ufbxStringToStd(ufbx_string s) {
	return std::string(s.data, s.length);
}

ModelLoadResult FbxModelLoader::load(const std::string& filepath) {
	ModelLoadResult result;

	ufbx_load_opts opts{};
	// Convert to Y-up, right-handed (matching typical Vulkan/glTF convention)
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.generate_missing_normals = true;

	ufbx_error error;
	ufbx_scene* scene = ufbx_load_file(filepath.c_str(), &opts, &error);
	if (!scene) {
		char errBuf[512];
		ufbx_format_error(errBuf, sizeof(errBuf), &error);
		throw std::runtime_error(std::string("[Spell] FBX: Failed to load: ") + errBuf);
	}

	std::string baseDir = std::filesystem::path(filepath).parent_path().string();
	if (baseDir.empty()) baseDir = ".";

	// Extract materials
	for (size_t i = 0; i < scene->materials.count; i++) {
		const ufbx_material* mat = scene->materials.data[i];
		MaterialInfo info{};

		// Try PBR properties first, fallback to FBX classic
		auto getTexPath = [&](const ufbx_material_map& pbrMap, const ufbx_material_map& fbxMap) -> std::string {
			if (pbrMap.texture && pbrMap.texture->type == UFBX_TEXTURE_FILE) {
				std::string raw = ufbxStringToStd(pbrMap.texture->relative_filename);
				if (raw.empty()) raw = ufbxStringToStd(pbrMap.texture->absolute_filename);
				return resolveTexturePath(raw, baseDir);
			}
			if (fbxMap.texture && fbxMap.texture->type == UFBX_TEXTURE_FILE) {
				std::string raw = ufbxStringToStd(fbxMap.texture->relative_filename);
				if (raw.empty()) raw = ufbxStringToStd(fbxMap.texture->absolute_filename);
				return resolveTexturePath(raw, baseDir);
			}
			return "";
		};

		info.diffuseTexturePath = getTexPath(mat->pbr.base_color, mat->fbx.diffuse_color);
		info.normalTexturePath = getTexPath(mat->pbr.normal_map, mat->fbx.normal_map);
		info.metallicTexturePath = getTexPath(mat->pbr.metalness, mat->fbx.specular_color);
		info.roughnessTexturePath = getTexPath(mat->pbr.roughness, mat->fbx.specular_exponent);

		result.materials.push_back(info);
	}

	std::cout << "[Spell] FBX: Loaded " << result.materials.size() << " material(s) from " << filepath << std::endl;

	// Process all meshes in the scene
	for (size_t mi = 0; mi < scene->meshes.count; mi++) {
		const ufbx_mesh* mesh = scene->meshes.data[mi];

		// Triangulate the mesh
		size_t maxTriIndices = mesh->max_face_triangles * 3;
		std::vector<uint32_t> triIndices(maxTriIndices);

		for (size_t fi = 0; fi < mesh->faces.count; fi++) {
			ufbx_face face = mesh->faces.data[fi];
			uint32_t numTris = ufbx_triangulate_face(triIndices.data(), maxTriIndices, mesh, face);

			// Determine material for this face
			int materialIndex = -1;
			if (mesh->face_material.count > 0) {
				uint32_t matIdx = mesh->face_material.data[fi];
				if (mesh->materials.count > 0 && matIdx < mesh->materials.count) {
					// Map mesh-local material to scene-global material index
					const ufbx_material* faceMat = mesh->materials.data[matIdx];
					for (size_t si = 0; si < scene->materials.count; si++) {
						if (scene->materials.data[si] == faceMat) {
							materialIndex = static_cast<int>(si);
							break;
						}
					}
				}
			}

			uint32_t baseVertex = static_cast<uint32_t>(result.vertices.size());

			for (uint32_t ti = 0; ti < numTris * 3; ti++) {
				uint32_t meshIndex = triIndices[ti];

				Vertex vertex{};

				ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, meshIndex);
				vertex.pos = { static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z) };

				if (mesh->vertex_normal.exists) {
					ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, meshIndex);
					vertex.normal = { static_cast<float>(normal.x), static_cast<float>(normal.y), static_cast<float>(normal.z) };
				} else {
					vertex.normal = { 0.0f, 0.0f, 1.0f };
				}

				if (mesh->vertex_uv.exists) {
					ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, meshIndex);
					vertex.texCoord = { static_cast<float>(uv.x), 1.0f - static_cast<float>(uv.y) };
				}

				vertex.color = { 1.0f, 1.0f, 1.0f };
				vertex.materialIndex = materialIndex;

				result.vertices.push_back(vertex);
				result.indices.push_back(baseVertex + ti);
			}
		}
	}

	ufbx_free_scene(scene);

	std::cout << "[Spell] FBX: " << result.vertices.size() << " vertices, "
		<< result.indices.size() << " indices" << std::endl;

	return result;
}

std::vector<MaterialInfo> FbxModelLoader::preParseTexturePaths(const std::string& filepath) {
	ufbx_load_opts opts{};
	opts.ignore_geometry = true;
	opts.ignore_animation = true;
	opts.ignore_embedded = true;

	ufbx_error error;
	ufbx_scene* scene = ufbx_load_file(filepath.c_str(), &opts, &error);
	if (!scene) return {};

	std::string baseDir = std::filesystem::path(filepath).parent_path().string();
	if (baseDir.empty()) baseDir = ".";

	std::vector<MaterialInfo> result;
	for (size_t i = 0; i < scene->materials.count; i++) {
		const ufbx_material* mat = scene->materials.data[i];
		MaterialInfo info{};

		auto getTexPath = [&](const ufbx_material_map& pbrMap, const ufbx_material_map& fbxMap) -> std::string {
			if (pbrMap.texture && pbrMap.texture->type == UFBX_TEXTURE_FILE) {
				std::string raw = ufbxStringToStd(pbrMap.texture->relative_filename);
				if (raw.empty()) raw = ufbxStringToStd(pbrMap.texture->absolute_filename);
				return resolveTexturePath(raw, baseDir);
			}
			if (fbxMap.texture && fbxMap.texture->type == UFBX_TEXTURE_FILE) {
				std::string raw = ufbxStringToStd(fbxMap.texture->relative_filename);
				if (raw.empty()) raw = ufbxStringToStd(fbxMap.texture->absolute_filename);
				return resolveTexturePath(raw, baseDir);
			}
			return "";
		};

		info.diffuseTexturePath = getTexPath(mat->pbr.base_color, mat->fbx.diffuse_color);
		info.normalTexturePath = getTexPath(mat->pbr.normal_map, mat->fbx.normal_map);
		info.metallicTexturePath = getTexPath(mat->pbr.metalness, mat->fbx.specular_color);
		info.roughnessTexturePath = getTexPath(mat->pbr.roughness, mat->fbx.specular_exponent);

		result.push_back(info);
	}

	ufbx_free_scene(scene);

	std::cout << "[Spell] FBX: Pre-parsed " << result.size() << " material(s) for texture paths" << std::endl;
	return result;
}

} // namespace Spell
