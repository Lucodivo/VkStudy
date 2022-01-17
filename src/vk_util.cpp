#include "vk_util.h"

#include <iostream>

#include "vk_initializers.h"

AllocatedBuffer vkutil::createBuffer(VmaAllocator& vmaAllocator, u64 allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage) {
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;


	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memUsage;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferInfo, &vmaallocInfo,
		&newBuffer.vkBuffer,
		&newBuffer.vmaAllocation,
		nullptr));

	return newBuffer;
}

void vkutil::immediateSubmit(const UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = uploadContext.commandBuffer;

	// This command buffer exactly once before resetting
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submitInfo(&cmd);

	VK_CHECK(vkQueueSubmit(uploadContext.queue, 1, &submit, uploadContext.uploadFence));

	vkWaitForFences(uploadContext.device, 1, &uploadContext.uploadFence, true, 9999999999);
	vkResetFences(uploadContext.device, 1, &uploadContext.uploadFence);

	// reset command buffers in command pool
	vkResetCommandPool(uploadContext.device, uploadContext.commandPool, 0);
}