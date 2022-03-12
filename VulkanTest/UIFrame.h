#pragma once
#ifndef UIFRAME_H
#define UIFRAME_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

static ImGui_ImplVulkanH_Window g_MainWindowData;

void initImGUI(GLFWwindow* window);

void createImGUIWindow(void);

void ImGUIBeginFrame(void);

void ImGUIEndFrame(void);

void ImGUIShutdown(void);

#endif
