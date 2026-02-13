#include "UIFrame.h"
#include <iostream>

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_QueueFamily = (uint32_t)-1;

static VkRenderPass		_imguiRenderPass;
static VkCommandPool	_imguiCommandPool;
static VkCommandBuffer	_imguiCommandBuffer;

static uint32_t w;
static uint32_t h;

static int                      g_MinImageCount = 2;

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void DoInit(VkDevice device) {
	
	// Create RenderPass
	{
		VkAttachmentDescription attachment = {};
		attachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		VkAttachmentReference color_attachment = {};
		color_attachment.attachment = 0;
		color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment;
		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;
		vkCreateRenderPass(device, &info, nullptr, &_imguiRenderPass);
	}

	// Create CommandPool && Commandbuffer
	{
		// todo
	}

	// todo Init FrameBuffer

	// todo Create Semaphore

}


void initImGUI(VkInstance instance, GLFWwindow* window, 
	VkDevice device, VkQueue queue, VkPhysicalDevice physicalDevice, 
	VkDescriptorPool descriptorPool,
	VkSurfaceKHR surface, VkAllocationCallbacks * g_Allocator) {

	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

	wd->Surface = surface;
	// Check for WSI support
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, g_QueueFamily, wd->Surface, &res);
	if (res != VK_TRUE)
	{
		fprintf(stderr, "Error no WSI support on physical device 0\n");
		exit(-1);
	}

	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(physicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	// Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif

	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

	// Create SwapChain, RenderPass, Framebuffer, etc.
	ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, wd, g_QueueFamily, g_Allocator, w, h, g_MinImageCount, 0);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	io.DisplaySize = ImVec2(400, 300);
	io.DeltaTime = 1.0f / 60.0f;
	ImGui_ImplGlfw_InitForVulkan(window, true);


	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.QueueFamily = g_QueueFamily;
	init_info.Queue = queue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = descriptorPool;
	init_info.MinImageCount = g_MinImageCount;
	init_info.ImageCount = wd->ImageCount;
	init_info.Allocator = g_Allocator;
	init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	ImGui_ImplVulkan_Init(&init_info);


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

