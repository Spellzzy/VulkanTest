#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace Spell {

class SpellWindow {
public:
	SpellWindow(int width, int height, const std::string& name);
	~SpellWindow();

	SpellWindow(const SpellWindow&) = delete;
	SpellWindow& operator=(const SpellWindow&) = delete;

	bool shouldClose() { return glfwWindowShouldClose(window_); }
	VkExtent2D getExtent() { return { static_cast<uint32_t>(width_), static_cast<uint32_t>(height_) }; }
	bool wasResized() { return framebufferResized_; }
	void resetResizedFlag() { framebufferResized_ = false; }
	GLFWwindow* getGLFWwindow() const { return window_; }

	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

private:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	int width_;
	int height_;
	bool framebufferResized_ = false;
	std::string windowName_;
	GLFWwindow* window_;

	void initWindow();
};

} // namespace Spell
