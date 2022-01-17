#pragma once

#include "vk_types.h"
#include "vk_mem_alloc.h"

namespace vkutil {

	bool loadImageFromFile(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char* file, AllocatedImage& outImage);

}
