#include "UIFrame.h"
#include <iostream>

void initImGUI(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	io.DisplaySize = ImVec2(400, 300);
	io.DeltaTime = 1.0f / 60.0f;
	ImGui_ImplGlfw_InitForVulkan(window, true);
	unsigned char* tex_pixels = NULL;
	int tex_w, tex_h;
	io.Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

	std::cout << "this is initImGUI \n";
}

void createImGUIWindow() {
	/*ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	wd->Surface = surface;
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.QueueFamily = 0;
	init_info.Queue = graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = VK_NULL_HANDLE;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = wd->ImageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = NULL;
	ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);


	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);*/
}

void ImGUIBeginFrame() {
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGUIEndFrame() {
	/*ImGui::Text("Hi~~~");

	ImGui::Render();*/
	//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), );
}

void ImGUIShutdown() {
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

