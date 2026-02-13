#include "SpellImGui.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>

namespace Spell {

SpellImGui::SpellImGui(SpellWindow& window, SpellDevice& device, VkRenderPass renderPass, uint32_t imageCount)
	: device_(device) {
	// 初始化 ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	// 加载中文字体
	ImFontConfig fontConfig;
	fontConfig.OversampleH = 2;
	fontConfig.OversampleV = 1;
	io.Fonts->AddFontFromFileTTF(
		"assets/msyh.ttc",
		16.0f,
		&fontConfig,
		io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
	);

	// 初始化 GLFW 后端
	ImGui_ImplGlfw_InitForVulkan(window.getGLFWwindow(), true);

	// 初始化 Vulkan 后端（新版 API：使用 DescriptorPoolSize 让 ImGui 自动创建 descriptor pool）
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.ApiVersion = VK_API_VERSION_1_0;
	initInfo.Instance = device_.getInstance();
	initInfo.PhysicalDevice = device_.physicalDevice();
	initInfo.Device = device_.device();

	QueueFamilyIndices queueFamilies = device_.findPhysicalQueueFamilies();
	initInfo.QueueFamily = queueFamilies.graphicsFamily.value();
	initInfo.Queue = device_.graphicsQueue();
	initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = imageCount;
	initInfo.PipelineInfoMain.RenderPass = renderPass;
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.MSAASamples = device_.msaaSamples();

	ImGui_ImplVulkan_Init(&initInfo);
}

SpellImGui::~SpellImGui() {
	vkDeviceWaitIdle(device_.device());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void SpellImGui::newFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void SpellImGui::render(VkCommandBuffer commandBuffer) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

} // namespace Spell
