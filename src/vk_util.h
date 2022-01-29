#pragma once

namespace vkutil {

  AllocatedBuffer createBuffer(VmaAllocator& vmaAllocator, u64 allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);
  void immediateSubmit(const UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);
  u64 padUniformBufferSize(const VkPhysicalDeviceProperties& gpuProperties, u64 originalSize);
  void loadShaderBuffer(const char* filePath, std::vector<char>& outBuffer);
  VkShaderModule loadShaderModule(VkDevice device, const char* filePath);
  VkShaderModule loadShaderModule(VkDevice device, std::vector<char>& fileBuffer);

  // Returns the size in bytes of the provided VkFormat.
  // As this is only intended for vertex attribute formats, not all VkFormats are supported.
  static u32 formatSize(VkFormat format);
}
