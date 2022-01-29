#pragma once

#define VK_CHECK(x)                                                 \
  do                                                              \
  {                                                               \
    VkResult err = x;                                           \
    if (err)                                                    \
    {                                                           \
      std::cout <<"Detected Vulkan error: " << err << std::endl; \
      abort();                                                \
    }                                                           \
  } while (0)

struct AllocatedBuffer {
  VkBuffer vkBuffer;
  VmaAllocation vmaAllocation;
};

struct AllocatedImage {
  VkImage vkImage;
  VmaAllocation vmaAllocation;
};

struct Texture {
  AllocatedImage image;
  VkImageView imageView;
};

struct UploadContext {
  VkDevice device;
  VkQueue queue;
  VkFence uploadFence;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
};

struct DeletionQueue {
  std::vector<std::function<void()>> deletors;

  void pushFunction(std::function<void()>&& function) {
    deletors.push_back(function);
  }

  void flush() {
    for(s64 i = (s64)deletors.size() - 1; i >= 0; i--) {
      deletors[i]();
    }
    deletors.clear();
  }
};