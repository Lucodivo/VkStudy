#pragma once

#include <vector>
#include <functional>

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#include "types.h"

struct AllocatedBuffer {
    VkBuffer vkBuffer;
    VmaAllocation vmaAllocation;
};

struct AllocatedImage {
  VkImage vkImage;
  VmaAllocation vmaAllocation;
};

struct DeletionQueue
{
	std::vector<std::function<void()>> deletors;

	void pushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		for (s32 i = deletors.size() - 1; i >= 0; i--) {
			deletors[i]();
		}
		deletors.clear();
	}
};