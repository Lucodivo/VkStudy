#pragma once

namespace vkutil {

  bool loadImageFromFile(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char* file, AllocatedImage& outImage);

}
