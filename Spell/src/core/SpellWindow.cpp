#include "SpellWindow.h"
#include <stdexcept>

namespace Spell {

SpellWindow::SpellWindow(int width, int height, const std::string& name)
	: width_(width), height_(height), windowName_(name) {
	initWindow();
}

SpellWindow::~SpellWindow() {
	glfwDestroyWindow(window_);
	glfwTerminate();
}

void SpellWindow::initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window_ = glfwCreateWindow(width_, height_, windowName_.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window_, this);
	glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

void SpellWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
	if (glfwCreateWindowSurface(instance, window_, nullptr, surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void SpellWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto spellWindow = reinterpret_cast<SpellWindow*>(glfwGetWindowUserPointer(window));
	spellWindow->framebufferResized_ = true;
	spellWindow->width_ = width;
	spellWindow->height_ = height;
}

} // namespace Spell
