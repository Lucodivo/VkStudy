#pragma once

VK_DEFINE_HANDLE(VkImguiInstance)

struct VkImguiCreateInfo {
  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice chosenGPU;
  VkRenderPass renderPass;
  VkCommandBuffer fontTextUploadCommandBuffer;
  VkQueue graphicsQueue;
  u32 graphicsQueueFamily;
  u32 swapChainImageCount; // TODO: rename
};

void vkImguiTextWindow(const char* title, const CStringRingBuffer& ringBuffer, bool& show);
void vkImguiCreateInstance(const VkImguiCreateInfo* ci, VkImguiInstance* pImguiInstance);
void vkImguiDeinit(VkImguiInstance imguiInstance);
void vkImguiStartFrame();
void vkImguiRender(VkCommandBuffer cmd);
void vkImguiQuickDebugFloat(bool showQuickDebug, const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags = 0);
void vkImguiQuickDebugText(bool showQuickDebug, const char* fmt, ...);