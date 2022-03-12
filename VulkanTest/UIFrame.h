#pragma once
#ifndef UIFRAME_H
#define UIFRAME_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void initImGUI(VkInstance instance, GLFWwindow* window,
	VkDevice device, VkQueue queue, VkPhysicalDevice physicalDevice,
	VkDescriptorPool descriptorPool,
	VkSurfaceKHR surface, VkAllocationCallbacks* g_Allocator);

void createImGUIWindow(void);

void ImGUIBeginFrame(void);

void ImGUIEndFrame(void);

void ImGUIShutdown(void);

#endif
