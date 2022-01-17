#pragma once

#include "vk_types.h"
#include "vk_mem_alloc.h"

namespace vkutil {

	AllocatedBuffer createBuffer(VmaAllocator& vmaAllocator, u64 allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage);
	void immediateSubmit(const UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function);
}
