#include "SpellInspector.h"
#include "renderer/SpellTypes.h"

#include <imgui.h>

namespace Spell {

bool SpellInspector::draw(SpellResourceManager& resources, LightPushConstantData& light, bool& convertYUp, const RenderStats& stats) {
	bool needReload = false;

	ImGui::Begin("Inspector");

	if (ImGui::CollapsingHeader("Render Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("FPS: %.1f (%.3f ms/frame)", stats.fps, stats.frameTimeMs);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Frames Per Second / Frame Time\n\n"
				"每秒帧数 / 每帧耗时\n"
				"衡量渲染性能的基本指标\n"
				"60 FPS (16.7ms) 为流畅标准");

		ImGui::Text("Draw Calls:  %u", stats.drawCalls);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Draw Calls\n\n"
				"绘制调用次数\n"
				"CPU 向 GPU 提交的绘制命令数量\n"
				"过多的 Draw Call 会成为 CPU 端瓶颈");

		ImGui::Text("Triangles:   %u", stats.triangles);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Triangles (CPU-side)\n\n"
				"三角形数量（CPU 端统计）\n"
				"由索引数 / 3 计算得出\n"
				"表示提交给 GPU 的三角形总数");

		ImGui::Text("Vertices:    %u", stats.vertices);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Vertices (CPU-side)\n\n"
				"顶点数量（CPU 端统计）\n"
				"模型顶点缓冲区中的顶点总数\n"
				"包含位置、法线、UV 等属性的唯一顶点");

		ImGui::Text("Indices:     %u", stats.indices);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Indices (CPU-side)\n\n"
				"索引数量（CPU 端统计）\n"
				"索引缓冲区中的索引总数\n"
				"通过索引复用顶点，减少显存占用");

		ImGui::Text("Textures:    %u", stats.textureCount);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Texture Count\n\n"
				"纹理数量\n"
				"当前已加载到 GPU 显存的纹理数量\n"
				"包括漫反射、法线、金属度、粗糙度等贴图");

		ImGui::Text("Materials:   %u", stats.materialCount);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Material Count\n\n"
				"材质数量\n"
				"模型使用的材质数量\n"
				"每个材质可关联多张纹理贴图");

		ImGui::Separator();
		ImGui::Text("Load Time:   %.1f ms", stats.totalLoadTimeMs);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Total Load Time\n\n"
				"资源总加载耗时\n"
				"包括模型加载和纹理加载的总时间\n"
				"切换模型后会更新");

		ImGui::Text("  Model:     %.1f ms", stats.modelLoadTimeMs);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Model Load Time\n\n"
				"模型加载耗时\n"
				"包括模型文件解析、顶点去重、\n"
				"顶点/索引缓冲区创建和 GPU 上传");

		ImGui::Text("  Textures:  %.1f ms", stats.textureLoadTimeMs);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Texture Load Time\n\n"
				"纹理加载耗时\n"
				"包括图片解码、Staging Buffer 创建、\n"
				"GPU 纹理上传和 Mipmap 生成");

		if (stats.decodeOverlapMs > 0.0f) {
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "  Overlap:  -%.0f ms", stats.decodeOverlapMs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Parallel Overlap Savings\n\n"
					"并行加载节省的时间\n"
					"纹理 CPU 解码与模型加载并行执行，\n"
					"此时间被隐藏在模型加载过程中");
		}
	}

	if (ImGui::CollapsingHeader("GPU Pipeline Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("IA Vertices:     %llu", stats.gpuIAVertices);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Input Assembly Vertices\n\n"
				"输入装配顶点数\n"
				"GPU 从顶点/索引缓冲区读取的顶点数量\n"
				"使用索引绘制时，计算的是索引引用次数（非去重顶点数）");

		ImGui::Text("IA Primitives:   %llu", stats.gpuIAPrimitives);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Input Assembly Primitives\n\n"
				"输入装配图元数\n"
				"从顶点组装出的三角形数量\n"
				"对于三角形列表: IA Vertices / 3");

		ImGui::Text("VS Invocations:  %llu", stats.gpuVSInvocations);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Vertex Shader Invocations\n\n"
				"顶点着色器调用次数\n"
				"通常小于 IA Vertices，因为 GPU 有变换后顶点缓存\n"
				"缓存命中率 = 1 - VS Invocations / IA Vertices");

		ImGui::Text("Clip Primitives: %llu", stats.gpuClippingPrimitives);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Clipping Primitives\n\n"
				"裁剪阶段输出图元数\n"
				"通过裁剪阶段(位于视锥体内)的图元数量\n"
				"若小于 IA Primitives，说明部分三角形被裁剪或剔除");

		ImGui::Text("FS Invocations:  %llu", stats.gpuFSInvocations);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Fragment Shader Invocations\n\n"
				"片段着色器调用次数\n"
				"光栅化后执行片段(像素)着色器的次数\n"
				"相对于屏幕分辨率过高可能意味着 overdraw 严重");
	}
	ImGui::Separator();

	ImGui::Checkbox("Convert Y-up to Z-up", &convertYUp);
	ImGui::Separator();

	ImGui::Text("Light");
	ImGui::ColorEdit3("Color", lightColor_);
	ImGui::DragFloat("Intensity", &lightIntensity_, 0.1f, 0.0f, 100.0f);
	light.color = glm::vec3(lightColor_[0], lightColor_[1], lightColor_[2]) * lightIntensity_;
	ImGui::DragFloat3("Position", &light.position.x, 0.1f, -10.0f, 10.0f);

	ImGui::Separator();
	ImGui::Text("Resources");

	auto& availableModels = resources.availableModels();
	auto& availableTextures = resources.availableTextures();

	// Model selector
	auto modelGetter = [](void* data, int idx, const char** out_text) -> bool {
		auto& models = *static_cast<const std::vector<std::string>*>(data);
		if (idx < 0 || idx >= static_cast<int>(models.size())) return false;
		*out_text = models[idx].c_str();
		return true;
	};
	ImGui::Combo("Model", &selectedModelIdx_, modelGetter,
		const_cast<void*>(static_cast<const void*>(&availableModels)),
		static_cast<int>(availableModels.size()));

	if (resources.model()) {
		ImGui::Text("  Vertices: %u  Indices: %u",
			resources.model()->getVertexCount(), resources.model()->getIndexCount());
		ImGui::Text("  Materials: %u  Textures: %u",
			static_cast<uint32_t>(resources.model()->getMaterials().size()),
			resources.textureCount());
	}

	// Fallback texture selector (used when model has no materials)
	bool hasMaterials = resources.model() && !resources.model()->getMaterials().empty();
	auto texGetter = [](void* data, int idx, const char** out_text) -> bool {
		auto& textures = *static_cast<const std::vector<std::string>*>(data);
		if (idx < 0 || idx >= static_cast<int>(textures.size())) return false;
		*out_text = textures[idx].c_str();
		return true;
	};
	if (hasMaterials) {
		ImGui::BeginDisabled();
	}
	ImGui::Combo("Fallback Tex", &selectedTextureIdx_, texGetter,
		const_cast<void*>(static_cast<const void*>(&availableTextures)),
		static_cast<int>(availableTextures.size()));
	if (hasMaterials) {
		ImGui::EndDisabled();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(model uses embedded textures)");
	}

	ImGui::Spacing();

	bool modelChanged = (!availableModels.empty() &&
		selectedModelIdx_ < static_cast<int>(availableModels.size()) &&
		availableModels[selectedModelIdx_] != resources.modelPath());
	bool textureChanged = (!availableTextures.empty() &&
		selectedTextureIdx_ < static_cast<int>(availableTextures.size()) &&
		availableTextures[selectedTextureIdx_] != resources.texturePath());

	if (modelChanged || textureChanged) {
		if (ImGui::Button("Apply Changes")) {
			if (modelChanged) resources.setModelPath(availableModels[selectedModelIdx_]);
			if (textureChanged) resources.setTexturePath(availableTextures[selectedTextureIdx_]);
			needReload = true;
		}
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "(pending changes)");
	}

	if (ImGui::Button("Force Reload")) {
		needReload = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh File List")) {
		resources.scanAvailableFiles();
		syncSelection(resources);
	}

	// Material texture info
	if (resources.model() && !resources.model()->getMaterials().empty()) {
		ImGui::Separator();
		ImGui::Text("Material Textures (auto-loaded from model):");
		const auto& mats = resources.model()->getMaterials();
		for (size_t i = 0; i < mats.size(); i++) {
			ImGui::Text("  [%zu] diffuse: %s", i,
				mats[i].diffuseTexturePath.empty() ? "(none)" : mats[i].diffuseTexturePath.c_str());
			ImGui::Text("       normal: %s",
				mats[i].normalTexturePath.empty() ? "(none)" : mats[i].normalTexturePath.c_str());
			ImGui::Text("       metallic: %s",
				mats[i].metallicTexturePath.empty() ? "(none)" : mats[i].metallicTexturePath.c_str());
			ImGui::Text("       roughness: %s",
				mats[i].roughnessTexturePath.empty() ? "(none)" : mats[i].roughnessTexturePath.c_str());
		}
	}

	ImGui::End();

	return needReload;
}

void SpellInspector::syncSelection(const SpellResourceManager& resources) {
	auto& models = resources.availableModels();
	auto& textures = resources.availableTextures();

	for (int i = 0; i < static_cast<int>(models.size()); i++) {
		if (models[i] == resources.modelPath()) { selectedModelIdx_ = i; break; }
	}
	for (int i = 0; i < static_cast<int>(textures.size()); i++) {
		if (textures[i] == resources.texturePath()) { selectedTextureIdx_ = i; break; }
	}
}

} // namespace Spell
