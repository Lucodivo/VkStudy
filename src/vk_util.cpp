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

u64 vkutil::padUniformBufferSize(const VkPhysicalDeviceProperties& gpuProperties, u64 originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	u64 alignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
	u64 alignedSize = originalSize;
	if (alignment > 0) {
		// As the alignment must be a power of 2...
		// (alignment - 1) creates a mask that is all 1s below the alignment
		// ~(alignment - 1) creates a mask that is all 1s at and above the alignment
		alignedSize = (originalSize + (alignment - 1)) & ~(alignment - 1);

		// Note: similar idea as above but less optimized below
		// memory_index alignmentCount = (original + (alignment - 1)) / alignment;
		// alignedSize = alignmentCount * alignment;
	}
	return alignedSize;
}