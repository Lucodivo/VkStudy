#pragma once

namespace vkutil {
  void loadImageFromAssetFile(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char* file, AllocatedImage& outImage);
  void loadImagesFromAssetFiles(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char** files, AllocatedImage* outImages, u32 imageCount);
}
