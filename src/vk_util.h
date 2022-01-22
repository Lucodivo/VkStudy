#pragma once

namespace vkutil {

	AllocatedBuffer createBuffer(VmaAllocator& vmaAllocator, u64 allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);
	void immediateSubmit(const UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);
	u64 padUniformBufferSize(const VkPhysicalDeviceProperties& gpuProperties, u64 originalSize);
	VkShaderModule loadShaderModule(VkDevice device, const char* filePath);
}
