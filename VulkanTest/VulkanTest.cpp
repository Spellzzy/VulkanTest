#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>

#include <chrono>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include <fstream>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

#include <cstdint>
#include <algorithm>

#include <glm/glm.hpp>
#include <array>

#include "UIFrame.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
//const std::string MODEL_PATH = "models/moshi.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

// 常量定义 同时处理多少帧
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool  enableValidationLayers = false;
#else
const bool  enableValidationLayers = true;
#endif // NDEBUG

// 全局标识符对象 UBO
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	//  上传GPU内存后将如何传递给顶点着色器
	static VkVertexInputBindingDescription getBindingDescription() {
		// 顶点绑定描述
		VkVertexInputBindingDescription bindingDesc{};
		// 指定了绑定数组中的索引 
		bindingDesc.binding = 0;
		// 下一个条目 字节数步长
		bindingDesc.stride = sizeof(Vertex);
		// VK_VERTEX_INPUT_RATE_VERTEX 移动到每个顶点之后的下一个数据条目 
		// VK_VERTEX_INPUT_RATE_INSTANCE 在每个实例之后移动到下一个数据条目 实例化渲染时使用
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDesc;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDesc{};
		attributeDesc[0].binding = 0;
		attributeDesc[0].location = 0;
		// 属性的数据类型
		/*	float： VK_FORMAT_R32_SFLOAT
			vec2：  VK_FORMAT_R32G32_SFLOAT
			vec3：  VK_FORMAT_R32G32B32_SFLOAT
			vec4：  VK_FORMAT_R32G32B32A32_SFLOAT*/
			/*ivec2: VK_FORMAT_R32G32_SINT, 32 位有符号整数的 2 分量向量
			uvec4 : VK_FORMAT_R32G32B32A32_UINT, 32 位无符号整数的 4 分量向量
			double : VK_FORMAT_R64_SFLOAT, 双精度（64 位）浮点数*/
		attributeDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[0].offset = offsetof(Vertex, pos);


		attributeDesc[1].binding = 0;
		attributeDesc[1].location = 1;
		attributeDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[1].offset = offsetof(Vertex, color);

		attributeDesc[2].binding = 0;
		attributeDesc[2].location = 2;
		attributeDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDesc[2].offset = offsetof(Vertex, texCoord);

		attributeDesc[3].binding = 0;
		attributeDesc[3].location = 3;
		attributeDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDesc[3].offset = offsetof(Vertex, normal);

		return attributeDesc;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}
//const std::vector<Vertex> vertices = {
//	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
//	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
//	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
//	{{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
//};

//const std::vector<Vertex> vertices = {
//	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
//};

//const std::vector<Vertex> vertices = {
//	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
//
//	{{-0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, 0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//	{{-0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
//
//	{{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
//	{{0.5f, 0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
//
//	{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//	{{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//	{{-0.5f, 0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
//	{{-0.5f, -0.5f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
//
//	{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
//	{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
//	{{-0.5f, -0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
//	{{0.5f, -0.5f, -1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
//
//	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
//	{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
//	{{0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
//	{{-0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
//};
//
//// uint16_t 65535
//const std::vector<uint16_t> indices = {	
//	0,1,2,2,3,0,
//	8,9,10,10,11,8,
//	12,13,14,14,15,12,
//	16,17,18,18,19,16,
//	20,21,22,22,23,20,
//	4,5,6,6,7,4
//};

struct LightPushConstantData {
	glm::vec3 color;
	glm::vec3 position;
};

class HelloTrangleApplication
{
public:
	void run() {

		initWindow();

		initImGUI(window);

		initVulkan();

		mainLoop();

		cleanup();
	}

private:
	GLFWwindow* window;
	// vk实例
	VkInstance instance;
	// 存储逻辑设备句柄
	VkDevice device;
	// 物例设备
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	// 绘制队列
	VkQueue graphicsQueue;
	// 演示队列
	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	// 存储图像视图
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;
	// 帧缓冲区
	std::vector<VkFramebuffer> swapChainFramebuffers;
	// 命令池
	VkCommandPool commandPool;
	// 命令缓冲区
	std::vector<VkCommandBuffer> commandBuffers;

	// 每帧都应该有自己的信号量
	// 用来表示 图像已被获取并准备好渲染
	std::vector<VkSemaphore> imageAvailableSemaphores;
	// 用来表示 渲染已经完成 并且可以进行渲染
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	size_t currentFrame = 0;

	bool framebufferResized = false;
	// 顶点缓冲区
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	// 索引缓冲区
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorSetLayout descriptorSetLayout;
	// 描述符池
	VkDescriptorPool descriptorPool;
	// 描述符集合
	std::vector<VkDescriptorSet> descriptorSets;	

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageView textureImageView;
	// 采样器对象
	VkSampler textureSampler;

	// 深度缓冲
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	// mipmap等级
	uint32_t mipLevels;
	// 采样数量
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	LightPushConstantData lightData{};

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		//GLFWwindow *window;
		window = glfwCreateWindow(WIDTH, HEIGHT, "Spell", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		// 设置size change回调
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTrangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		// 创建vk实例
		createInstance();
		createSurface();
		// 收集设备信息
		pickPhysicalDevice();
		// 创建逻辑设备
		createLogicalDevice();
		// 创建交换链
		createSwapChain();
		// 创建图像视图
		createImageViews();
		// 创建渲染通道
		createRenderPass();
		// 创建描述符布局输送 准备
		createDescriptorSetLayout();
		// 创建图形流水线
		createGraphicsPipeline();
		// 创建采样颜色缓冲区 
		createColorResources();
		// 创建深度图像视图
		createDepthResources();
		// 创建帧缓冲区
		createFramebuffers();
		// 创建命令池
		createCommandPool();
		// 创建纹理图像
		createTextureImage();
		// 创建图像视图
		createTextureImageView();
		// 创建采样器对象
		createTextureSampler();

		loadModel();

		// 创建顶点缓冲区
		createVertexBuffer();
		// 创建索引缓冲区
		createIndexBuffer();
		// 创建描述符缓冲区
		createUniformBuffers();
		// 创建描述符池
		createDescriptorPool();
		// 做描述符分配
		createDescriptorSets();
		// 创建命令缓冲区
		createCommandBuffers();
		// 创建所需信号 同步
		createSyncObjects();
	}

	void createInstance() {
		// 验证
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("需要验证层 但未找到");
		}


		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "draw triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "None";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;


		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			std::cout << "enabledLayerCount" << createInfo.enabledLayerCount;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			std::cout << "enabledLayerCount 0000" << createInfo.enabledLayerCount;
		}

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("创建实例失败");
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::cout << "available extensions:\n";

		for (const auto& extension : extensions)
		{
			std::cout << '\t' << extension.extensionName << "\n";
		}

	}

	void createSurface() {
		// 如果是离屏渲染 可以不需要此步
		// vulkan不依赖窗口 (openGL 依赖)
		// 此处glfw 为我们处理了平台差异
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("创建 window surface 失败");
		}
	}

	// 逻辑设备不直接与实例交互 所以没有参数
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);


		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}



		//VkDeviceQueueCreateInfo queueCreateInfo{};
		//queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		//// 单个queueFamily的队列数量
		//// 实际并不需要多个 开一个 然后开多线程
		//queueCreateInfo.queueCount = 1;
		//// 设置队列优先级 0.0~1.0之间
		//float queuePriority = 1.0f;
		//queueCreateInfo.pQueuePriorities = &queuePriority;

		// 指定使用的设备功能
		
		// 获取物理设备 支持的功能
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
		// 开启样本采样
		deviceFeatures.sampleRateShading = VK_TRUE;
		// 创建 deviceCreateInfo
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		// 添加指向队列创建信息和设备功能结构的指针
		//createInfo.pQueueCreateInfos = &queueCreateInfo;
		//createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		// 所需外部扩展数量 这些都是特定于设备的
		//createInfo.enabledExtensionCount = 0;

		// 启用设备扩展
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		// 实例化设备
		// 交互的物理设备， 指定的队列和使用信息， 可分配回调指针， 指向存储逻辑设备句柄变量指针
		// 销毁时 需要vkDestroyDevice() 对应销毁
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)!= VK_SUCCESS)
		{
			throw std::runtime_error("创建逻辑设备时出错");
		}

		// 检索每个队列Family中的队列Handle
		// DeviceQueue 会在销毁时 自动隐式清理 不需要手动清理
		// 只创建一个队列 所以索引为0
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		// 演示队列
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		/*VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;*/

	}

	struct SwapChainSupportDetails
	{
		// 基本表面功能(最小/最大图像数 图像最小/最大宽高)
		VkSurfaceCapabilitiesKHR capabilities;
		// 表面格式
		std::vector<VkSurfaceFormatKHR> formats;
		// 可用的present Mode
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice curDevice) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(curDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(curDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(curDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(curDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(curDevice, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		//简单地坚持这个最小值意味着我们有时可能不得不等待驱动程序完成内部操作，然后才能获取另一个图像进行渲染。因此，建议请求至少比最小值多一张图像
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // 指定层级里每个图像包含的数量

		//  表示 图像如何使用  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT 为 直接进行渲染
		// VK_IMAGE_USAGE_TRANSFER_DST_BIT  渲染为单独图像 来进行后处理操作 可以使用内存操作将渲染图像传输到交换链
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			// 图像可以跨多个 queueFamily 无需明确所有权
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			// 提前指定将在那些队列系列中之间共享图像所有权
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			// 一个图像一次只由一个 queueFamily拥有 在转移到其他队列时 必须明确转移所有权 该选项可提供最佳性能
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		// 可以指定对交换链中的图像应用某种变换 例如旋转 或者水平翻转
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		// 是否使用Alpha通道与其他窗口混合
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // OPAQUE

		createInfo.presentMode = presentMode;
		// 为true时 不关心被遮挡的像素颜色  如果需要知道这些颜色信息 设置false
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;


		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error(" failed to create swap chain~~");
		}

		// 我们仅在交换链中指定了最小数量的图像，因此实现允许创建具有更多图像的交换链
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		// 为交换链中图像 选择格式和范围
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void createImageViews() {
		// 调整列表大小
		swapChainImageViews.resize(swapChainImages.size());

		// 设置所有的交换链图像
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			// 转移到 createImageView中
			//VkImageViewCreateInfo createInfo{};
			//createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			//createInfo.image = swapChainImages[i];
			//// 设置 视图纹理类型 1D 2D 3D Cube
			//createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			//createInfo.format = swapChainImageFormat;

			//// components 允许进行混合颜色通道
			//// 例如，您可以将所有通道映射到单色纹理的红色通道。您也可以映射的常量0和1一个通道。在我们的例子中，我们将坚持默认映射
			//createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			//createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			//createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			//createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			//// subresourceRange字段描述了图像的用途以及应该访问图像的哪个部分。我们的图像将用作没有任何 mipmapping 级别或多层的颜色目标。
			//// 如果您正在处理立体 3D 应用程序，那么您将创建一个具有多个层的交换链。然后，您可以通过访问不同的层，为代表左眼和右眼视图的每个图像创建多个图像视图。
			//createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//createInfo.subresourceRange.baseMipLevel = 0;
			//createInfo.subresourceRange.levelCount = 1;
			//createInfo.subresourceRange.baseArrayLayer = 0;
			//createInfo.subresourceRange.layerCount = 1;

			//if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			//{
			//	throw std::runtime_error("failed to create image views !!!");
			//}
		}
	}

	// 每个VkSurfaceFormatKHR 包含 
	// 一个format 指定颜色通道和类型 比如 R8G8B8A8_SRGB 8位无符号整数顺序RGBA通道 每个像素32位
	// 和一个colorspace  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR标志指示是否支持 SRGB 颜色空间
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// VK_PRESENT_MODE_FIFO_KHR 交换链队列 在显示器准备刷新时 从队列前面取出图像 程序在队列后插入图像 如果队列满了 程序必须等待
		// VK_PRESENT_MODE_MAILBOX_KHR 三重缓冲  队列满了 不等待 将队列里排队的图像换成 较新的图像
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
			};

			// 约束下宽高
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("there is no vulkan device support");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			std::cout << "in pickPhysicalDevice \n ";
			if (isDeviceSuitable(device))
			{
				// 赋值使用的设备信息
				physicalDevice = device;
				msaaSamples = getMaxUsableSampleCount();
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU.");
		}

	}

	// 判断device gpu 是否为合适的支持硬件
	bool isDeviceSuitable(VkPhysicalDevice curDevice) {

		/*VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(curDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(curDevice, &deviceFeatures);
		
		std::cout << "deviceProperties.deviceType --> " << deviceProperties.deviceType;
		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;*/

		QueueFamilyIndices indices = findQueueFamilies(curDevice);
		// 扩展支持
		bool extensionSupported = checkDeviceExtensionSupport(curDevice);
		// swapChain 支持
		bool swapChainAdequate = false;
		if (extensionSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(curDevice);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(curDevice, &supportedFeatures);

		return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;

		//return true;
	}

	// 检查 swapchain 支持
	bool checkDeviceExtensionSupport(VkPhysicalDevice curDevice)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(curDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(curDevice, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		
		return requiredExtensions.empty();
	}

	struct QueueFamilyIndices
	{
		// std::optional 是一个包装容器 初始不包含任何值 直到为其分配值
		// 支持绘图命令 supporting drawing commands
		std::optional<uint32_t> graphicsFamily;
		// 支持演示队列  supporting presentation
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}

	};
	// 查找图形命令队列
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice curDevice) {
		QueueFamilyIndices indices;
		// todo 查找逻辑

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(curDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(curDevice, &queueFamilyCount, queueFamilies.data());
		std::cout << "in findQueueFamilies " << queueFamilyCount << std::endl;

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			// 至少需要支持 VK_QUEUE_GRAPHICS_BIT
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(curDevice, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			// 存在一个可用设备即可 跳出
			if (indices.isComplete())
			{
				break;
			}

			i++;
		}
		return indices;
	}


	void mainLoop() {
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		// 退出前 先等待逻辑设备完成操作
		vkDeviceWaitIdle(device);

		//ImGUIShutdown();
	}


	void drawFrame() {
		//ImGUIBeginFrame();

		// 等待新号完成
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		
		// 从交换链中获取图像
		// 使用该图像作为帧缓冲区中的attachment  执行命令缓冲区
		// 将图像返回到交换链中进行展示
		uint32_t imageIndex;
		// UINT64_MAX 可以禁用超时
		// imageAvailableSemaphore 完成时的信号同步对象
		// imageIndex 输出可用的交换链图像索引
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// 交换链与表面不兼容 无法再用于渲染  通常在窗口大小变化后
			recreateSwapChain();
			return;
		}
		// VK_SUBOPTIMAL_KHR 仍可以使用交换链呈现到表面 但表面属性已经不完全匹配
		else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("交换链呈现图像出错！");
		}

		updateUniformBuffer(imageIndex);

		//currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		// 如果前一帧是使用的该image
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		// 记录该帧使用的image
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// 定义了执行开始之前 要等待的信号量以及要等待的管道阶段
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// 重置栅栏恢复状态
		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// 最后一个参数引用了一个可选的栅栏，当命令缓冲区完成执行时，该栅栏将发出信号。
		// 我们使用信号量进行同步，所以我们只需要传递一个VK_NULL_HANDLE.
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS )
		{
			throw std::runtime_error("提交 绘图命令时出错！！！");
		}				

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		// 指定要设显示图像的交换链以及每个交换连的图像索引
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		// 如果展示成果 允许指定一组值来检查每个单独的交换链 如果只有一个交换链 则没必要 应为可以获取返回值嘛
		//presentInfo.pResults = nullptr;
	
		// 提交请求 将图像呈现给交换链
		vkQueuePresentKHR(presentQueue, &presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		
		//ImGUIEndFrame();
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;

		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);

		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0 )
				{
					layerFound = true;
					break;
				}			
			}
			if (!layerFound)
			{
				return false;
			}
		}
		return true;
	}


	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("打开文件 失败啦！！！");
		}
		
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	// 流水线创建
	void createGraphicsPipeline() {
	
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 顶点输入
		// 描述了将要传递给顶点着色器顶点的数据结构
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		auto bindingDesc = Vertex::getBindingDescription();
		auto attrtbuteDesc = Vertex::getAttributeDescriptions();


		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// 绑定 数据之间的间距以及数据时按顶点还是按实例来进行
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
		// 属性描述 传递给顶点着色器的属性类型 绑定从哪个offset 来进行加载
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrtbuteDesc.size());
		vertexInputInfo.pVertexAttributeDescriptions = attrtbuteDesc.data();

		// 输入程序组件
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		/*VK_PRIMITIVE_TOPOLOGY_POINT_LIST: 从顶点点
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST：每 2 个顶点的线，无需重用
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : 每行的结束顶点用作下一行的开始顶点
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST：每3个顶点的三角形，无需重用
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : 每个三角形的第二个和第三个顶点用作下一个三角形的前两个顶点*/
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// If you set the primitiveRestartEnable member to VK_TRUE, 
		// then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// 视口
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		// 深度值范围 在0.0-1.0之间
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// 裁剪窗口
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		// 视口与裁剪窗口 组合
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// 光栅化
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// 如果depthClampEnable设置为VK_TRUE，则超出近平面和远平面的片段将被夹在它们上面，而不是丢弃它们。
		// 这在一些特殊情况下很有用，比如阴影贴图。使用它需要启用 GPU 功能。
		rasterizer.depthClampEnable = VK_FALSE;
		// 如果rasterizerDiscardEnable设置为VK_TRUE，则几何体永远不会通过光栅化阶段。这基本上禁用了帧缓冲区的任何输出。
		rasterizer.rasterizerDiscardEnable = VK_FALSE;

		// 用于生成几何
		// VK_POLYGON_MODE_FILL：用片段填充多边形区域
		// VK_POLYGON_MODE_LINE: 多边形边被绘制为线
		// VK_POLYGON_MODE_POINT: 多边形顶点绘制为点
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// 根据片段的数量描述了线条的粗细
		rasterizer.lineWidth = 1.0f;

		// 剔除 可以禁止剔除 剔除正面 剔除背面 或者两者全部
		// 剔除背面  
		//VK_CULL_MODE_NONE
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		// 指定要被视为正面的顶点顺序 顺时针或逆时针
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// 多重采样
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		// TODO
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.rasterizationSamples = msaaSamples;
		multisampling.minSampleShading = 0.2f; // 缩小样本采样最小单位
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// 深度测试 模板测试
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		// 开启深度测试
		depthStencil.depthTestEnable = VK_TRUE;
		// 开启深度写入
		depthStencil.depthWriteEnable = VK_TRUE;
		// 对比方式 更低更近 应该写入
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		// 模板测试
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		// 颜色混合
		// 片段着色器返回后 需要和帧缓冲区中已有的颜色组合
		// 混合旧值和新值 / 按位运算组合旧值与新值
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		// color mask
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// Blend mode
		// 设置false时 来自frag的新颜色 未经检查修改 就通过了
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;		

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


		// 混合逻辑
		/*if (blendEnable) {
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) < colorBlendOp > (dstColorBlendFactor * oldColor.rgb);
			finalColor.a = (srcAlphaBlendFactor * newColor.a) < alphaBlendOp > (dstAlphaBlendFactor * oldColor.a);
		}
		else {
			finalColor = newColor;
		}

		finalColor = finalColor & colorWriteMask;


		finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
		finalColor.a = newAlpha.a;

		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		*/

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		// logicOpEnable 为 true 是 可以指定logicOp 按位运算
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// 动态状态  需要动态变化的数据
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		// 推送常量
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(LightPushConstantData);

		// Pipeline layout 在此阶段 定义全局uniform		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// !!!!!! 找了一下午 之前 设置好描述符布局后 count 忘记设置了 还是0
		// 会在 createCommandBuffers 阶段时 vkcmdbinddescriptorSets后 commandbuffer 指针丢失 出错
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		// todo
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS )
		{
			throw std::runtime_error("创建流水线布局时 出错！");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		// 开始组装流水线
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // todo

		pipelineInfo.layout = pipelineLayout;
		// 绑定渲染通道
		pipelineInfo.renderPass = renderPass;
		// 子通道索引
		pipelineInfo.subpass = 0;

		// vulkan 允许通过现有管道派生创建新的图形管道
		// 当管道与现有管道有共同功能时 设置管道成本会更低 并且可以更快地在来自同一父管道的管道之间切换
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("创建图形流水线 失败！！");
		}


		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	// 创建着色器模块
	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		// 讲char为单位的data 转换为uint32
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS )
		{
			throw std::runtime_error("创建着色器模块 失败！！！");
		}
		return shaderModule;
	}

	// 渲染通道
	void createRenderPass(){
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = msaaSamples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// 目前暂不关心 存储的深度数据
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// 不关心之前的深度内容 所以initlayout 为 undefined
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment{};
		// 匹配交换链图像格式
		colorAttachment.format = swapChainImageFormat;
		// 设置采样数量
		colorAttachment.samples = msaaSamples;
		// 渲染前操作
		/*VK_ATTACHMENT_LOAD_OP_LOAD: 保留附件的现有内容
		VK_ATTACHMENT_LOAD_OP_CLEAR：在开始时将值清除为常量
		VK_ATTACHMENT_LOAD_OP_DONT_CARE : 现有内容未定义；我们不在乎他们*/
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// 渲染后操作
		// VK_ATTACHMENT_STORE_OP_STORE：渲染的内容将存储在内存中，以后可以读取
		// VK_ATTACHMENT_STORE_OP_DONT_CARE : 渲染操作后帧缓冲区的内容将是未定义的
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;


		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：用作彩色附件的图像 适合作为从片段着色器写入颜色的附件
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：要在交换链中呈现的图像
		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : 用作内存复制操作目标的图像
		// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 适合从着色器中采样
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// 我们希望图像在渲染后准备好使用交换链进行展示
		//colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// 此时需要做采样操作 所以不能直接呈现图像
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachmentResolve{};
		colorAttachmentResolve.format = swapChainImageFormat;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPass{};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPass.colorAttachmentCount = 1;
		// 颜色附件引用
		subPass.pColorAttachments = &colorAttachmentRef;
		// 指定依赖项
		subPass.pDepthStencilAttachment = &depthAttachmentRef;

		subPass.pResolveAttachments = &colorAttachmentResolveRef;
		//pInputAttachments: Attachments that are read from a shader
		//pResolveAttachments : Attachments used for multisampling color attachments
		//pDepthStencilAttachment : Attachment for depthand stencil data
		//pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved

		VkSubpassDependency dependency{};
		// 指定依赖项
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		// 从属子通道的索引
		dependency.dstSubpass = 0;
		// 指定要等待的操作 以及操作发生的阶段
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("创建渲染通道失败!");
		}

	}

	void createFramebuffers() {
		// 与交换链 同步容量大小
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			std::array<VkImageView, 3> attachments = {
				colorImageView,
				depthImageView,
				swapChainImageViews[i]
			};

			// 创建帧缓冲区内容
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			// 单个图像 所以层数为1
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS )
			{
				throw std::runtime_error(" 创建 frame buffer 时 失败！");
			}

		}
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS )
		{
			throw std::runtime_error("创建 命令池 失败！");
		}
	}

	void createColorResources() {
		VkFormat colorFormat = swapChainImageFormat;

		createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

		colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	
	
	}

	void createDepthResources() {
		VkFormat depthFormat = findDepthFormat();

		createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			depthImage, depthImageMemory);
		depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	}

	VkFormat findDepthFormat() {
		return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);	
	}

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format: candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features )
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("未找到合适的 格式");
	}

	// 检查是否存在模板测试
	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		// 指定分配的命令缓冲区是主命令缓冲区还是辅助命令缓冲区		
		// VK_COMMAND_BUFFER_LEVEL_PRIMARY 可以提交到队列执行 但不能从其他命令缓冲区调用
		// VK_COMMAND_BUFFER_LEVEL_SECONDARY 不能直接提交 但是可以从主命令缓冲区调用
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS )
		{
			throw std::runtime_error("分配命令缓冲区 出错！");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			// 设置如何使用该命令缓冲区
			// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT  在执行一次后立即重新记录
			// VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT 这是一个辅助命令缓冲区 将完全在单个渲染通道中
			// VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT 命令缓冲区可以在它已经挂起执行时 重新提交
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS )
			{
				throw std::runtime_error("启用命令缓冲区时 出错");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;

			// 因为 loadOp 阶段定义了 CLEAR
			// 所以需要定义要使用的清除颜色
			std::array<VkClearValue, 2> clearValues{};
			// 顺序需要与绑定附件顺序相同
			// 颜色附件的清除值
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			// 深度附件的清除值
			// 在深度缓冲器深度范围是0.0对1.0在Vulkan，
			// 其中1.0 位于在远视点平面和0.0在近视点平面。
			// 深度缓冲区中每个点的初始值应该是最远的可能深度，即1.0。
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			// 所有命令函数都是  vkCmd开头 都为void函数

			// 第一个参数都是 将命令记录到的commandbuffer 
			// 第二个参数指定我们刚提供的渲染通道的详细信息
			// 第三个参数 指定如何提供渲染通道中的绘图命令  
			//	VK_SUBPASS_CONTENTS_INLINE 渲染通道命令将嵌入主命令缓冲区本身 不会执行辅助命令缓冲区
			//	VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS 命令将从辅助命令缓冲区执行
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// 绑定图形管道
			// 第二个参数指定管道对象是 图形管道还是计算管道
				//vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// 绘制
			// vertexCount：即使我们没有顶点缓冲区，从技术上讲，我们仍然有 3 个顶点要绘制。
			// instanceCount：用于实例化渲染，1如果您不这样做，请使用。
			// firstVertex：用作顶点缓冲区的偏移量，定义 的最小值gl_VertexIndex。
			// firstInstance：用作实例渲染的偏移量，定义 的最低值gl_InstanceIndex。
			//vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);			

			lightData.color = glm::vec3(1.0, 1.0, 0.0);
			lightData.position = glm::vec3(2.0, 2.0, 2.0);
			// VK_SHADER_STAGE_FRAGMENT_BIT 只应用在片段着色器阶段
			vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightPushConstantData), &lightData);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			// 使用顶点缓冲区内信息进行绘制
			//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			// 描述符集不是图形管道独有的 所以 需要制定是都要将描述符集绑定到图形或者计算管道 VK_PIPELINE_BIND_POINT_GRAPHICS
			// pipelineLayout 描述符所基于的布局
			// 后三个参数 指定第一个描述符集的索引 要绑定的集数 和 要绑定的集数组
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
				&descriptorSets[i], 0, nullptr);

			

			// 使用索引绘制
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error(" 记录 命令缓冲区失败！");
			}
		}
	}

	void createSyncObjects() {

		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// vkWaitForFences 如果之前没有使用过它会一直等待
		// 在此处设置 在创建时 以存在信号状态下做创建 相当于一个完成的初始帧
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo,nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("创建信号量时 失败！");
			}
		}
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createColorResources();
		createDepthResources();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		// 做描述符分配
		createDescriptorSets();
		createCommandBuffers();

		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
	}

	void createVertexBuffer() {
#pragma region 转移到createBuffer里去做了
		
		//VkBufferCreateInfo bufferInfo{};
		//bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		//// 定义缓冲区大小
		//bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		//// 缓冲区中数据用途
		//bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		//// 访问权限 此处设置成了独占访问
		//bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertextBuffer) != VK_SUCCESS )
		//{
		//	throw std::runtime_error("创建顶点缓冲区 失败");
		//}

		//VkMemoryRequirements memRequirements;
		//vkGetBufferMemoryRequirements(device, vertextBuffer, &memRequirements);

		//VkMemoryAllocateInfo allocInfo{};
		//allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//allocInfo.allocationSize = memRequirements.size;
		//allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		//if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
		//{
		//	throw std::runtime_error("顶点缓冲区分配内存失败！");
		//}
		//// 将分配的内存与缓冲区相关联
		//// 若偏移量非0 则需要被 memRequirements.alignment 整除
		//vkBindBufferMemory(device, vertextBuffer, vertexBufferMemory, 0);
#pragma endregion

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		// VK_BUFFER_USAGE_TRANSFER_SRC_BIT 缓冲区可以用作内存传输操作中的源
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		// 填充顶点缓冲区
		// 映射内存的指针
		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);
		// VK_BUFFER_USAGE_TRANSFER_DST_BIT 标记缓冲区可用作内存传输操作中的目的地
		// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 
		// 标记了vertextBuffer是从设备本地的内存类型分配的 所以不能使用vkMapMemory 需要做copy操作
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		//vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		//VkCommandBufferBeginInfo beginInfo{};
		//beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		//vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		// 开始copy
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);

		//// 停止记录
		//vkEndCommandBuffer(commandBuffer);
		//// 进行传输
		//VkSubmitInfo submitInfo{};
		//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		//submitInfo.commandBufferCount = 1;
		//submitInfo.pCommandBuffers = &commandBuffer;
		//// VK_NULL_HANDLE 此时没有需要等待的事件  直接立即执行传输
		//vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		//vkQueueWaitIdle(graphicsQueue);
		//// 清理已传输完成的commandbuffer
		//vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("查找是和缓冲区本身的内存类型 失败");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("创建buffer 失败");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS )
		{
			throw std::runtime_error("分配内存时出错");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void createIndexBuffer() {
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);
		// 注意 这里是indexbuffer VK_BUFFER_USAGE_INDEX_BUFFER_BIT
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	// 创建描述符 输送准备工作
	void createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;	// 描述符数量 
		// 指定在哪个着色器阶段引用该 描述符
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		// 与图像采样相关
		uboLayoutBinding.pImmutableSamplers = nullptr;		

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		// 片段着色器阶段引用
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout)!= VK_SUCCESS)
		{
			throw std::runtime_error("创建描述符布局失败");
		}
	}

	void createUniformBuffers() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(swapChainImages.size());
		uniformBuffersMemory.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		}
	}

	// 创建描述符池
	void createDescriptorPool() {
		/*VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());*/

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
		
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("创建描述符池 失败");
		}
	}

	// 分配描述符
	void createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();
		
		descriptorSets.resize(swapChainImages.size());
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS )
		{
			throw std::runtime_error("分配描述符时 出错");
		}

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			//VkWriteDescriptorSet descriptorWrite{};
			//descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			//descriptorWrite.dstSet = descriptorSets[i];
			//// 描述符可以是数组 所以还要给出数组中第一个索引  没使用数组 所以为0
			//descriptorWrite.dstBinding = 0;
			//descriptorWrite.dstArrayElement = 0;

			//descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			//// 指定要更新元素数量
			//descriptorWrite.descriptorCount = 1;
			//// 实际配置描述符的引用缓冲区
			//descriptorWrite.pBufferInfo = &bufferInfo;
			//// 用于引用Image数据的描述符
			//descriptorWrite.pImageInfo = nullptr;
			//// 用于引用texture数据的描述符
			//descriptorWrite.pTexelBufferView = nullptr;
			////  It accepts two kinds of arrays as parameters: an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet. 
			////  The latter can be used to copy descriptors to each other, as its name implies.
			//vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		}

	}

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		// 1D 2D 3D
		// 一维可用于存储数据或者梯度数组
		// 二维只要用作纹理 三维可用于存储体素体积
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;

		imageInfo.format = format;
		// VK_IMAGE_TILING_OPTIMAL：Texels按照实现定义的顺序排列 用于着色器的高效访问
		// VK_IMAGE_TILING_LINEAR Texels将按pixels数组一样 按优先顺序排序 方便访问 纹素
		imageInfo.tiling = tiling;

		// VK_IMAGE_LAYOUT_UNDEFINED：不可用于GPU 每个第一次转换后会舍弃texels 纹素
		// VK_IMAGE_LAYOUT_PREINITIALIZED 不可用于GPU 每个第一次转换后会保留texels 纹素
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		imageInfo.usage = usage;

		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		// 多重采样相关
		imageInfo.samples = numSamples;
		imageInfo.flags = 0;
		
		if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("创建图像时 失败");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("分配 图像内存时 失败");
		}
		vkBindImageMemory(device, image, imageMemory, 0);
		
	}

	void createTextureImage() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		//stbi_uc* pixels = stbi_load("textures/texture2.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4; // rgba
		if (!pixels)
		{
			throw std::runtime_error("加载图片时出错！");
		}

		// 计算mip链的级别数 开2次方  floor来处理最大维度不是2的幂的问题  +1 是加上原始图像level
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
		// 清理原始像素阵列
		stbi_image_free(pixels);

		createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		// 转移到 createImage 中了
		//VkImageCreateInfo imageInfo{};
		//imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		//// 1D 2D 3D
		//// 一维可用于存储数据或者梯度数组
		//// 二维只要用作纹理 三维可用于存储体素体积
		//imageInfo.imageType = VK_IMAGE_TYPE_2D;
		//imageInfo.extent.width = static_cast<uint32_t>(texWidth);
		//imageInfo.extent.height = static_cast<uint32_t>(texHeight);
		//imageInfo.extent.depth = 1;
		//imageInfo.mipLevels = 1;
		//imageInfo.arrayLayers = 1;
		//imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		//// VK_IMAGE_TILING_OPTIMAL：Texels按照实现定义的顺序排列 用于着色器的高效访问
		//// VK_IMAGE_TILING_LINEAR Texels将按pixels数组一样 按优先顺序排序 方便访问 纹素
		//imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		//// VK_IMAGE_LAYOUT_UNDEFINED：不可用于GPU 每个第一次转换后会舍弃texels 纹素
		//// VK_IMAGE_LAYOUT_PREINITIALIZED 不可用于GPU 每个第一次转换后会保留texels 纹素
		//imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		//imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//// 多重采样相关
		//imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		//imageInfo.flags = 0;
		//if (vkCreateImage(device, &imageInfo, nullptr, &textureImage)!= VK_SUCCESS)
		//{
		//	throw std::runtime_error("创建图像时 失败");
		//}
		//VkMemoryRequirements memRequirements;
		//vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
		//VkMemoryAllocateInfo allocInfo{};
		//allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//allocInfo.allocationSize = memRequirements.size;
		//allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory)!= VK_SUCCESS)
		//{
		//	throw std::runtime_error("分配 图像内存时 失败");
		//}
		//vkBindImageMemory(device, textureImage, textureImageMemory, 0);

		transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		copyBufferToImage(stagingBuffer, textureImage,
			static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		// 准备一个过渡为 着色器访问做准备
		/*transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);	*/

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);

		generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
	}

	void createTextureImageView() {
		textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	}

	// 创建采样器对象
	void createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		// 指定了如何插值 放大或者微缩
		// 放大涉及过采样 VK_FILTER_NEAREST 缩小涉及欠采样 VK_FILTER_LINEAR
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		// 轴向寻址模式
		//VK_SAMPLER_ADDRESS_MODE_REPEAT：超出图像尺寸时重复纹理。
		//VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT : 类似于重复，但在超出尺寸时反转坐标以镜像图像。
		//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE：取图像尺寸以外最接近坐标的边缘颜色。
		//VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE：类似于夹到边，而是使用与最近边相对的边。
		//VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER：采样超出图像尺寸时返回纯色。
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// 是否使用各向异性过滤
		samplerInfo.anisotropyEnable = VK_TRUE;

		// 查询硬件支持的 可用于计算你最终颜色的纹素样本数量
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		// 设置max数量 数量越大效果质量越高 但是要注意性能效率
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		// 图像之外采样时返回的颜色  不能指定任意颜色
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		// 设置true 可以指定 区间范围内的纹素  false 则为0，1 所有范围内的纹素寻址
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		// 是否开启比较功能
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		// mipmap设置
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		//samplerInfo.minLod = static_cast<float>(mipLevels / 2);
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		// 获取当前LOD Level 的伪代码
		//lod = getLodLevelFromScreenSize(); //smaller when the object is close, may be negative
		//lod = clamp(lod + mipLodBias, minLod, maxLod);

		//level = clamp(floor(lod), 0, texture.mipLevels - 1);  //clamped to the number of mip levels in the texture

		//if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
		//	color = sample(level);
		//}
		//else {
		//	color = blend(sample(level), sample(level + 1));
		//}

		if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler)!= VK_SUCCESS )
		{
			throw std::runtime_error("创建采样器对象时 失败");
		}
	}

	void cleanSwapChain() {
		vkDestroyImageView(device, colorImageView, nullptr);
		vkDestroyImage(device, colorImage, nullptr);
		vkFreeMemory(device, colorImageMemory, nullptr);


		vkDestroyImageView(device, depthImageView, nullptr);
		vkDestroyImage(device, depthImage, nullptr);
		vkFreeMemory(device, depthImageMemory, nullptr);

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		// 清理现有命令缓冲区 避免重新创建的性能消耗
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		// 优先销毁交换链
		vkDestroySwapchainKHR(device, swapChain, nullptr);

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	}

	void cleanup() {
		cleanSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		/*vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);*/

		vkDestroyCommandPool(device, commandPool, nullptr);

		/*for (auto framebuff : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuff, nullptr);
		}		*/

		vkDestroyDevice(device, nullptr);
		// 先销毁surface
		vkDestroySurfaceKHR(instance, surface, nullptr);
		// 再销毁 instance
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		
		VkImageView imageView;

		if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			throw std::runtime_error("创建图像视图失败");
		}
	
		return imageView;
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region);
	
		endSingleTimeCommands(commandBuffer);
	}

	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;
		//barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (hasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}


		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			// 读取发生在 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT 前
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			throw std::invalid_argument("不支持的 layout transition");
		}

		vkCmdPipelineBarrier(commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);


		endSingleTimeCommands(commandBuffer);

	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	
		UniformBufferObject ubo{};

		// 模型变换
		// glm::rotate 参数 享有的变换 旋转角度 旋转轴
		ubo.model = glm::rotate((glm::mat4(1.0f) * sin(time) * glm::mat4(0.5) + glm::mat4(0.5)), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		
		// 视角变换
		// glm::lookat 参数 眼睛位置 中心位置 向上的轴向
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		// 透視
		// 角度 屏幕寬高比 近裁剪平面 远裁剪平面距离
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
		// GLM最初是为OpenGL设计的 在此需要对Y轴 做倒转
		ubo.proj[1][1] *= -1;

		void* data;
		vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
		
	}	

	void loadModel() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()) != VK_SUCCESS )
		{
			throw std::runtime_error(warn + err);
		}
		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices )
			{
				Vertex vertex{};				

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]

				};
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f,1.0f,1.0f };

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
				//std::cout << "normal --> " << vertex.normal.r;
				//vertices.push_back(vertex);
				//indices.push_back(indices.size());
				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
	
	}

	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			throw std::runtime_error("该纹理格式 不支持线性过滤");
		}

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;

		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth,mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);


			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1)
			{
				mipWidth /= 2;
			}
			if (mipHeight > 1)
			{
				mipHeight /= 2;
			}
		}

		// 结束命令缓冲取之前 再插入一个管道barrier
		// 将mip级别转换为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	VkSampleCountFlagBits getMaxUsableSampleCount() {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	
		VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
			physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		std::cout << "in getMaxUsableSampleCount " << counts << std::endl;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

};

int main() {
	HelloTrangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}