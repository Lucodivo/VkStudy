
#include "vk_util.h"

// Note: Flags of Interest
// VkBufferUsageFlags:
// - VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
// - VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
// - VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Useful for temporary staging buffers
// VmaMemoryUsage:
// - VMA_MEMORY_USAGE_CPU_TO_GPU: Useful for uniform/storage buffers
// - VMA_MEMORY_USAGE_CPU_ONLY: Useful for temporary staging buffers
// VkMemoryPropertyFlags:
// - VK_MEMORY_PROPERTY_HOST_CACHED_BIT: Useful if the CPU may read need to read from the buffer (ex: decompressing with LZ4 directly in buffer)
AllocatedBuffer vkutil::createBuffer(VmaAllocator& vmaAllocator, u64 allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, VkMemoryPropertyFlags preferredMemoryFlags) {
  //allocate vertex buffer
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;

  bufferInfo.size = allocSize;
  bufferInfo.usage = usage;
  
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memUsage;
  vmaallocInfo.preferredFlags = preferredMemoryFlags;

  AllocatedBuffer newBuffer;

  //allocate the buffer
  VK_CHECK(vmaCreateBuffer(vmaAllocator, &bufferInfo, &vmaallocInfo,
                           &newBuffer.vkBuffer,
                           &newBuffer.vmaAllocation,
                           nullptr));

  return newBuffer;
}

void vkutil::immediateSubmit(const UploadContext& uploadContext, std::function<void(VkCommandBuffer cmd)>&& function) {
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

u64 vkutil::padUniformBufferSize(const VkPhysicalDeviceProperties& gpuProperties, u64 originalSize) {
  // Calculate required alignment based on minimum device offset alignment
  u64 alignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
  u64 alignedSize = originalSize;
  if(alignment > 0) {
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

void vkutil::loadShaderBuffer(const char* filePath, std::vector<char>& outBuffer) {
  readFile(filePath, outBuffer);

  // TODO: Is this necessary or will it always just be a multiple of 4?
  u64 alignedSize = ((outBuffer.size() + (4 - 1)) / 4) * 4;
  outBuffer.resize(alignedSize); // add extra bytes if necessary
}

VkShaderModule vkutil::loadShaderModule(VkDevice device, const char* filePath) {
  std::vector<char> buffer;
  loadShaderBuffer(filePath, buffer);
  VkShaderModule result = loadShaderModule(device, buffer);

  if(result == VK_NULL_HANDLE) {
    std::cout << "Opened file but couldn't create shader: " << filePath << std::endl;
  } else {
    std::cout << "Successfully created shader: " << filePath << std::endl;
  }

  return result;
}

VkShaderModule vkutil::loadShaderModule(VkDevice device, std::vector<char>& fileBuffer) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.codeSize = fileBuffer.size(); // must be a multiple of 4
  createInfo.pCode = (u32*)(fileBuffer.data()); // Vulkan expects SPIR-V to be in u32 array

  //check that the creation goes well
  VkShaderModule shaderModule = VK_NULL_HANDLE;
  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

  return shaderModule;
}

static u32 vkutil::formatSize(VkFormat format) {
  u32 result = 0;
  switch (format) {
  case VK_FORMAT_UNDEFINED: result = 0; break;
  case VK_FORMAT_R4G4_UNORM_PACK8: result = 1; break;
  case VK_FORMAT_R4G4B4A4_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_B4G4R4A4_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_R5G6B5_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_B5G6R5_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_R5G5B5A1_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_B5G5R5A1_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_A1R5G5B5_UNORM_PACK16: result = 2; break;
  case VK_FORMAT_R8_UNORM: result = 1; break;
  case VK_FORMAT_R8_SNORM: result = 1; break;
  case VK_FORMAT_R8_USCALED: result = 1; break;
  case VK_FORMAT_R8_SSCALED: result = 1; break;
  case VK_FORMAT_R8_UINT: result = 1; break;
  case VK_FORMAT_R8_SINT: result = 1; break;
  case VK_FORMAT_R8_SRGB: result = 1; break;
  case VK_FORMAT_R8G8_UNORM: result = 2; break;
  case VK_FORMAT_R8G8_SNORM: result = 2; break;
  case VK_FORMAT_R8G8_USCALED: result = 2; break;
  case VK_FORMAT_R8G8_SSCALED: result = 2; break;
  case VK_FORMAT_R8G8_UINT: result = 2; break;
  case VK_FORMAT_R8G8_SINT: result = 2; break;
  case VK_FORMAT_R8G8_SRGB: result = 2; break;
  case VK_FORMAT_R8G8B8_UNORM: result = 3; break;
  case VK_FORMAT_R8G8B8_SNORM: result = 3; break;
  case VK_FORMAT_R8G8B8_USCALED: result = 3; break;
  case VK_FORMAT_R8G8B8_SSCALED: result = 3; break;
  case VK_FORMAT_R8G8B8_UINT: result = 3; break;
  case VK_FORMAT_R8G8B8_SINT: result = 3; break;
  case VK_FORMAT_R8G8B8_SRGB: result = 3; break;
  case VK_FORMAT_B8G8R8_UNORM: result = 3; break;
  case VK_FORMAT_B8G8R8_SNORM: result = 3; break;
  case VK_FORMAT_B8G8R8_USCALED: result = 3; break;
  case VK_FORMAT_B8G8R8_SSCALED: result = 3; break;
  case VK_FORMAT_B8G8R8_UINT: result = 3; break;
  case VK_FORMAT_B8G8R8_SINT: result = 3; break;
  case VK_FORMAT_B8G8R8_SRGB: result = 3; break;
  case VK_FORMAT_R8G8B8A8_UNORM: result = 4; break;
  case VK_FORMAT_R8G8B8A8_SNORM: result = 4; break;
  case VK_FORMAT_R8G8B8A8_USCALED: result = 4; break;
  case VK_FORMAT_R8G8B8A8_SSCALED: result = 4; break;
  case VK_FORMAT_R8G8B8A8_UINT: result = 4; break;
  case VK_FORMAT_R8G8B8A8_SINT: result = 4; break;
  case VK_FORMAT_R8G8B8A8_SRGB: result = 4; break;
  case VK_FORMAT_B8G8R8A8_UNORM: result = 4; break;
  case VK_FORMAT_B8G8R8A8_SNORM: result = 4; break;
  case VK_FORMAT_B8G8R8A8_USCALED: result = 4; break;
  case VK_FORMAT_B8G8R8A8_SSCALED: result = 4; break;
  case VK_FORMAT_B8G8R8A8_UINT: result = 4; break;
  case VK_FORMAT_B8G8R8A8_SINT: result = 4; break;
  case VK_FORMAT_B8G8R8A8_SRGB: result = 4; break;
  case VK_FORMAT_A8B8G8R8_UNORM_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_SNORM_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_USCALED_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_UINT_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_SINT_PACK32: result = 4; break;
  case VK_FORMAT_A8B8G8R8_SRGB_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_SNORM_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_USCALED_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_UINT_PACK32: result = 4; break;
  case VK_FORMAT_A2R10G10B10_SINT_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_SNORM_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_USCALED_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_UINT_PACK32: result = 4; break;
  case VK_FORMAT_A2B10G10R10_SINT_PACK32: result = 4; break;
  case VK_FORMAT_R16_UNORM: result = 2; break;
  case VK_FORMAT_R16_SNORM: result = 2; break;
  case VK_FORMAT_R16_USCALED: result = 2; break;
  case VK_FORMAT_R16_SSCALED: result = 2; break;
  case VK_FORMAT_R16_UINT: result = 2; break;
  case VK_FORMAT_R16_SINT: result = 2; break;
  case VK_FORMAT_R16_SFLOAT: result = 2; break;
  case VK_FORMAT_R16G16_UNORM: result = 4; break;
  case VK_FORMAT_R16G16_SNORM: result = 4; break;
  case VK_FORMAT_R16G16_USCALED: result = 4; break;
  case VK_FORMAT_R16G16_SSCALED: result = 4; break;
  case VK_FORMAT_R16G16_UINT: result = 4; break;
  case VK_FORMAT_R16G16_SINT: result = 4; break;
  case VK_FORMAT_R16G16_SFLOAT: result = 4; break;
  case VK_FORMAT_R16G16B16_UNORM: result = 6; break;
  case VK_FORMAT_R16G16B16_SNORM: result = 6; break;
  case VK_FORMAT_R16G16B16_USCALED: result = 6; break;
  case VK_FORMAT_R16G16B16_SSCALED: result = 6; break;
  case VK_FORMAT_R16G16B16_UINT: result = 6; break;
  case VK_FORMAT_R16G16B16_SINT: result = 6; break;
  case VK_FORMAT_R16G16B16_SFLOAT: result = 6; break;
  case VK_FORMAT_R16G16B16A16_UNORM: result = 8; break;
  case VK_FORMAT_R16G16B16A16_SNORM: result = 8; break;
  case VK_FORMAT_R16G16B16A16_USCALED: result = 8; break;
  case VK_FORMAT_R16G16B16A16_SSCALED: result = 8; break;
  case VK_FORMAT_R16G16B16A16_UINT: result = 8; break;
  case VK_FORMAT_R16G16B16A16_SINT: result = 8; break;
  case VK_FORMAT_R16G16B16A16_SFLOAT: result = 8; break;
  case VK_FORMAT_R32_UINT: result = 4; break;
  case VK_FORMAT_R32_SINT: result = 4; break;
  case VK_FORMAT_R32_SFLOAT: result = 4; break;
  case VK_FORMAT_R32G32_UINT: result = 8; break;
  case VK_FORMAT_R32G32_SINT: result = 8; break;
  case VK_FORMAT_R32G32_SFLOAT: result = 8; break;
  case VK_FORMAT_R32G32B32_UINT: result = 12; break;
  case VK_FORMAT_R32G32B32_SINT: result = 12; break;
  case VK_FORMAT_R32G32B32_SFLOAT: result = 12; break;
  case VK_FORMAT_R32G32B32A32_UINT: result = 16; break;
  case VK_FORMAT_R32G32B32A32_SINT: result = 16; break;
  case VK_FORMAT_R32G32B32A32_SFLOAT: result = 16; break;
  case VK_FORMAT_R64_UINT: result = 8; break;
  case VK_FORMAT_R64_SINT: result = 8; break;
  case VK_FORMAT_R64_SFLOAT: result = 8; break;
  case VK_FORMAT_R64G64_UINT: result = 16; break;
  case VK_FORMAT_R64G64_SINT: result = 16; break;
  case VK_FORMAT_R64G64_SFLOAT: result = 16; break;
  case VK_FORMAT_R64G64B64_UINT: result = 24; break;
  case VK_FORMAT_R64G64B64_SINT: result = 24; break;
  case VK_FORMAT_R64G64B64_SFLOAT: result = 24; break;
  case VK_FORMAT_R64G64B64A64_UINT: result = 32; break;
  case VK_FORMAT_R64G64B64A64_SINT: result = 32; break;
  case VK_FORMAT_R64G64B64A64_SFLOAT: result = 32; break;
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32: result = 4; break;
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: result = 4; break;

  default:
    break;
  }
  return result;
}
