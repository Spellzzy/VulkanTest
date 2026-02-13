#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "ObjModelLoader.h"
#include "robin_hood.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace Spell {

ModelLoadResult ObjModelLoader::load(const std::string& filepath) {
	ModelLoadResult result;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::string mtlBaseDir = std::filesystem::path(filepath).parent_path().string();
	if (mtlBaseDir.empty()) mtlBaseDir = ".";
	mtlBaseDir += "/";

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlBaseDir.c_str())) {
		throw std::runtime_error(warn + err);
	}

	for (const auto& mat : materials) {
		MaterialInfo info{};
		if (!mat.diffuse_texname.empty()) {
			info.diffuseTexturePath = mtlBaseDir + mat.diffuse_texname;
		}
		if (!mat.bump_texname.empty()) {
			info.normalTexturePath = mtlBaseDir + mat.bump_texname;
		}
		if (!mat.metallic_texname.empty()) {
			info.metallicTexturePath = mtlBaseDir + mat.metallic_texname;
		}
		if (!mat.roughness_texname.empty()) {
			info.roughnessTexturePath = mtlBaseDir + mat.roughness_texname;
		}
		result.materials.push_back(info);
	}

	std::cout << "[Spell] OBJ: Loaded " << result.materials.size() << " material(s) from " << filepath << std::endl;
	for (size_t i = 0; i < result.materials.size(); i++) {
		std::cout << "  [" << i << "] diffuse: "
			<< (result.materials[i].diffuseTexturePath.empty() ? "(none)" : result.materials[i].diffuseTexturePath)
			<< ", normal: "
			<< (result.materials[i].normalTexturePath.empty() ? "(none)" : result.materials[i].normalTexturePath)
			<< ", metallic: "
			<< (result.materials[i].metallicTexturePath.empty() ? "(none)" : result.materials[i].metallicTexturePath)
			<< ", roughness: "
			<< (result.materials[i].roughnessTexturePath.empty() ? "(none)" : result.materials[i].roughnessTexturePath) << std::endl;
	}

	bool hasNormals = !attrib.normals.empty();

	size_t totalIndices = 0;
	for (const auto& shape : shapes) {
		totalIndices += shape.mesh.indices.size();
	}
	result.vertices.reserve(totalIndices / 3);
	result.indices.reserve(totalIndices);

	robin_hood::unordered_map<Vertex, uint32_t, std::hash<Vertex>> uniqueVertices{};
	uniqueVertices.reserve(totalIndices);
	for (const auto& shape : shapes) {
		size_t indexOffset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			int faceVerts = shape.mesh.num_face_vertices[f];
			int matId = -1;
			if (f < shape.mesh.material_ids.size()) {
				matId = shape.mesh.material_ids[f];
			}
			int materialIndex = (matId >= 0 && matId < static_cast<int>(result.materials.size())) ? matId : -1;

			glm::vec3 faceNormal{0.0f, 0.0f, 1.0f};
			if (!hasNormals && faceVerts >= 3) {
				const auto& i0 = shape.mesh.indices[indexOffset + 0];
				const auto& i1 = shape.mesh.indices[indexOffset + 1];
				const auto& i2 = shape.mesh.indices[indexOffset + 2];
				glm::vec3 p0 = {attrib.vertices[3*i0.vertex_index+0], attrib.vertices[3*i0.vertex_index+1], attrib.vertices[3*i0.vertex_index+2]};
				glm::vec3 p1 = {attrib.vertices[3*i1.vertex_index+0], attrib.vertices[3*i1.vertex_index+1], attrib.vertices[3*i1.vertex_index+2]};
				glm::vec3 p2 = {attrib.vertices[3*i2.vertex_index+0], attrib.vertices[3*i2.vertex_index+1], attrib.vertices[3*i2.vertex_index+2]};
				glm::vec3 edge1 = p1 - p0;
				glm::vec3 edge2 = p2 - p0;
				glm::vec3 n = glm::cross(edge1, edge2);
				float len = glm::length(n);
				if (len > 1e-6f) {
					faceNormal = n / len;
				}
			}

			for (int v = 0; v < faceVerts; v++) {
				const auto& index = shape.mesh.indices[indexOffset + v];
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (index.texcoord_index >= 0) {
					vertex.texCoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				}

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (hasNormals && index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				} else {
					vertex.normal = faceNormal;
				}

				vertex.materialIndex = materialIndex;

				auto [it, inserted] = uniqueVertices.try_emplace(vertex, static_cast<uint32_t>(result.vertices.size()));
				if (inserted) {
					result.vertices.push_back(vertex);
				}
				result.indices.push_back(it->second);
			}
			indexOffset += faceVerts;
		}
	}

	if (!hasNormals) {
		std::cout << "[Spell] OBJ: Model had no normals, computed flat face normals" << std::endl;
	}

	return result;
}

std::vector<MaterialInfo> ObjModelLoader::preParseTexturePaths(const std::string& filepath) {
	std::string mtlBaseDir = std::filesystem::path(filepath).parent_path().string();
	if (mtlBaseDir.empty()) mtlBaseDir = ".";
	mtlBaseDir += "/";

	std::vector<std::string> mtlFiles;
	{
		std::ifstream objFile(filepath);
		if (objFile.is_open()) {
			std::string line;
			while (std::getline(objFile, line)) {
				if (line.size() > 7 && line.substr(0, 7) == "mtllib ") {
					std::string mtlName = line.substr(7);
					while (!mtlName.empty() && (mtlName.back() == '\r' || mtlName.back() == '\n' || mtlName.back() == ' '))
						mtlName.pop_back();
					if (!mtlName.empty())
						mtlFiles.push_back(mtlBaseDir + mtlName);
				}
				if (!line.empty() && line[0] == 'v' && (line.size() == 1 || line[1] == ' ' || line[1] == 't' || line[1] == 'n'))
					break;
			}
		}
	}

	std::vector<MaterialInfo> result;
	for (const auto& mtlPath : mtlFiles) {
		std::ifstream mtlFile(mtlPath);
		if (!mtlFile.is_open()) continue;

		MaterialInfo* current = nullptr;
		std::string line;
		while (std::getline(mtlFile, line)) {
			while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
				line.pop_back();

			if (line.size() > 7 && line.substr(0, 7) == "newmtl ") {
				result.push_back({});
				current = &result.back();
			} else if (current) {
				auto extractFilename = [](const std::string& value) -> std::string {
					auto pos = value.rfind(' ');
					if (pos != std::string::npos && pos + 1 < value.size())
						return value.substr(pos + 1);
					return value;
				};

				if (line.size() > 7 && line.substr(0, 7) == "map_Kd ") {
					current->diffuseTexturePath = mtlBaseDir + extractFilename(line.substr(7));
				}
				else if (line.size() > 9 && (line.substr(0, 9) == "map_Bump " || line.substr(0, 9) == "map_bump ")) {
					current->normalTexturePath = mtlBaseDir + extractFilename(line.substr(9));
				}
				else if (line.size() > 5 && line.substr(0, 5) == "bump ") {
					current->normalTexturePath = mtlBaseDir + extractFilename(line.substr(5));
				}
				else if (line.size() > 7 && line.substr(0, 7) == "map_Pm ") {
					current->metallicTexturePath = mtlBaseDir + extractFilename(line.substr(7));
				}
				else if (line.size() > 7 && line.substr(0, 7) == "map_Pr ") {
					current->roughnessTexturePath = mtlBaseDir + extractFilename(line.substr(7));
				}
			}
		}
	}

	std::cout << "[Spell] OBJ: Pre-parsed " << result.size() << " material(s) from .mtl for texture paths" << std::endl;
	return result;
}

} // namespace Spell
