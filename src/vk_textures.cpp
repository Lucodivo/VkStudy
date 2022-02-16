bool vkutil::loadImageFromFile(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char* file, AllocatedImage& outImage) {
  s32 texWidth, texHeight, texChannels;

  stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  if(!pixels) {
    std::cout << "Failed to load texture file " << file << std::endl;
    return false;
  }

  void* pixelPtr = pixels;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  //the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

  //allocate temporary buffer for holding texture data to upload
  AllocatedBuffer stagingBuffer = vkutil::createBuffer(vmaAllocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void* data;
  vmaMapMemory(vmaAllocator, stagingBuffer.vmaAllocation, &data);
  memcpy(data, pixelPtr, imageSize);
  vmaUnmapMemory(vmaAllocator, stagingBuffer.vmaAllocation);

  stbi_image_free(pixels);

  VkExtent3D imageExtent;
  imageExtent.width = texWidth;
  imageExtent.height = texHeight;
  imageExtent.depth = 1;

  VkImageCreateInfo imgCreateInfo = vkinit::imageCreateInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

  AllocatedImage newImage;

  VmaAllocationCreateInfo imgAllocCreateInfo = {};
  imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  //allocate and create the image
  vmaCreateImage(vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &newImage.vkImage, &newImage.vmaAllocation, nullptr);

  vkutil::immediateSubmit(uploadContext, [&](VkCommandBuffer cmd) {
    // which aspects of the image are being accessed?
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // color data
    range.baseMipLevel = 0; // mip level
    range.levelCount = 1;
    range.baseArrayLayer = 0; // image array index
    range.layerCount = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier_toTransfer.pNext = nullptr;
    imageBarrier_toTransfer.srcAccessMask = 0;
    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image is in undefined layout
    imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // to be transformed to transfer destination layout
    imageBarrier_toTransfer.srcQueueFamilyIndex = 0;
    imageBarrier_toTransfer.dstQueueFamilyIndex = 0;
    imageBarrier_toTransfer.image = newImage.vkImage;
    imageBarrier_toTransfer.subresourceRange = range;

    // vkCmdPipelineBarrier defines memory dependencies between commands submitted before and after it
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, // dependency flags
                         0, nullptr, // memory barrier count and array
                         0, nullptr, // buffer memory barrier count and array
                         1, &imageBarrier_toTransfer); // image memory barrier count and array

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    // If either are 0, buffer memory is considered to be tightly packed according to the imageExtent
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = imageExtent;

    //copy the buffer into the image
    vkCmdCopyBufferToImage(cmd, stagingBuffer.vkBuffer, newImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
    imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // barrier the image into the shader readable layout
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
  });

  vmaDestroyBuffer(vmaAllocator, stagingBuffer.vkBuffer, stagingBuffer.vmaAllocation);

  std::cout << "Texture loaded successfully " << file << std::endl;

  outImage = newImage;

  return true;
}

VkFormat getVkFormat(const assets::TextureInfo& textureInfo) {
  switch(textureInfo.textureFormat) {
    case assets::TextureFormat::RGBA8:
      return VK_FORMAT_R8G8B8A8_SRGB;
      break;
    default: // guess
      return VK_FORMAT_R8G8B8A8_SRGB;
  }
}

void vkutil::loadImageFromAssetFile(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char* file, AllocatedImage& outImage) {
  assets::AssetFile assetFile{};
  loadAssetFile(file, &assetFile);

  assets::TextureInfo textureInfo{};
  readTextureInfo(assetFile, &textureInfo);

  //allocate temporary buffer for holding texture data to upload
  AllocatedBuffer stagingBuffer = vkutil::createBuffer(vmaAllocator, textureInfo.textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void* data;
  vmaMapMemory(vmaAllocator, stagingBuffer.vmaAllocation, &data);
  assets::unpackTexture(textureInfo, assetFile.binaryBlob.data(), assetFile.binaryBlob.size(), (char*)data);
  vmaUnmapMemory(vmaAllocator, stagingBuffer.vmaAllocation);

  VkExtent3D imageExtent;
  imageExtent.width = textureInfo.width;
  imageExtent.height = textureInfo.height;
  imageExtent.depth = 1;

  AllocatedImage newImage;
  newImage.vkFormat = getVkFormat(textureInfo);
  VkImageCreateInfo imgCreateInfo = vkinit::imageCreateInfo(newImage.vkFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);


  VmaAllocationCreateInfo imgAllocCreateInfo = {};
  imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  //allocate and create the image
  vmaCreateImage(vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &newImage.vkImage, &newImage.vmaAllocation, nullptr);

  vkutil::immediateSubmit(uploadContext, [&](VkCommandBuffer cmd) {
    // which aspects of the image are being accessed?
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // color data
    range.baseMipLevel = 0; // mip level
    range.levelCount = 1;
    range.baseArrayLayer = 0; // image array index
    range.layerCount = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier_toTransfer.pNext = nullptr;
    imageBarrier_toTransfer.srcAccessMask = 0;
    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image is in undefined layout
    imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // to be transformed to transfer destination layout
    imageBarrier_toTransfer.srcQueueFamilyIndex = 0;
    imageBarrier_toTransfer.dstQueueFamilyIndex = 0;
    imageBarrier_toTransfer.image = newImage.vkImage;
    imageBarrier_toTransfer.subresourceRange = range;

    // vkCmdPipelineBarrier defines memory dependencies between commands submitted before and after it
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, // dependency flags
                         0, nullptr, // memory barrier count and array
                         0, nullptr, // buffer memory barrier count and array
                         1, &imageBarrier_toTransfer); // image memory barrier count and array

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    // If either are 0, buffer memory is considered to be tightly packed according to the imageExtent
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = imageExtent;

    //copy the buffer into the image
    vkCmdCopyBufferToImage(cmd, stagingBuffer.vkBuffer, newImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
    imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // barrier the image into the shader readable layout
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
  });

  vmaDestroyBuffer(vmaAllocator, stagingBuffer.vkBuffer, stagingBuffer.vmaAllocation);

  std::cout << "Texture loaded successfully " << file << std::endl;

  outImage = newImage;
}

// TODO: Any sort of error handling?
void vkutil::loadImagesFromAssetFiles(VmaAllocator& vmaAllocator, const UploadContext& uploadContext, const char** files, AllocatedImage* outImages, u32 imageCount) {
  std::vector<assets::AssetFile> assetFiles;
  assetFiles.resize(imageCount);
  std::vector<assets::TextureInfo> textureInfos;
  textureInfos.resize(imageCount);
  std::vector<VkExtent3D> imageExtents;
  imageExtents.resize(imageCount);

  VmaAllocationCreateInfo imgAllocCreateInfo = {};
  imgAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  u32 stagingBufferSize = 0;
  for(u32 i = 0; i < imageCount; i++) {
    assets::AssetFile& assetFile = assetFiles[i];
    assets::TextureInfo& textureInfo = textureInfos[i];
    VkExtent3D& imageExtent = imageExtents[i];

    loadAssetFile(files[i], &assetFile);
    readTextureInfo(assetFile, &textureInfo);
    imageExtent.width = textureInfo.width;
    imageExtent.height = textureInfo.height;
    imageExtent.depth = 1;

    AllocatedImage& allocImage = outImages[i];
    allocImage.vkFormat = getVkFormat(textureInfo);

    //allocate and create the image
    VkImageCreateInfo imgCreateInfo = vkinit::imageCreateInfo(allocImage.vkFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    vmaCreateImage(vmaAllocator, &imgCreateInfo, &imgAllocCreateInfo, &allocImage.vkImage, &allocImage.vmaAllocation, nullptr);

    stagingBufferSize += textureInfos[i].textureSize;
  }

  AllocatedBuffer stagingVMABuffer = vkutil::createBuffer(vmaAllocator, stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void* data;
  vmaMapMemory(vmaAllocator, stagingVMABuffer.vmaAllocation, &data);
    u32 stagingBufferPtrIter = 0;
    for(u32 i = 0; i < imageCount; i++) {
      const assets::TextureInfo& texInfo = textureInfos[i];
      const assets::AssetFile& assetFile = assetFiles[i];
      assets::unpackTexture(texInfo, assetFile.binaryBlob.data(), assetFile.binaryBlob.size(), ((char*)data) + stagingBufferPtrIter);
      stagingBufferPtrIter += texInfo.textureSize;
    }
  vmaUnmapMemory(vmaAllocator, stagingVMABuffer.vmaAllocation);

  vkutil::immediateSubmit(uploadContext, [imageCount, &stagingVMABuffer, &outImages, &imageExtents, &textureInfos](VkCommandBuffer cmd) {
    // which aspects of the image will be accessed?
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // color data
    range.baseMipLevel = 0; // mip level
    range.levelCount = 1;
    range.baseArrayLayer = 0; // image array index
    range.layerCount = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier_toTransfer.pNext = nullptr;
    imageBarrier_toTransfer.srcAccessMask = 0;
    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // image is in undefined layout
    imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // to be transformed to transfer destination layout
    imageBarrier_toTransfer.srcQueueFamilyIndex = 0;
    imageBarrier_toTransfer.dstQueueFamilyIndex = 0;
    imageBarrier_toTransfer.subresourceRange = range;

    VkBufferImageCopy copyRegion = {};
    // If either are 0, buffer memory is considered to be tightly packed according to the imageExtent
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};

    VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
    imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    u32 stagingBufferOffset = 0;
    for(u32 i = 0; i < imageCount; i++) {
      AllocatedImage& allocImage = outImages[i];
      VkExtent3D& imageExtent = imageExtents[i];
      assets::TextureInfo& textureInfo = textureInfos[i];

      imageBarrier_toTransfer.image = allocImage.vkImage;

      // vkCmdPipelineBarrier defines memory dependencies between commands submitted before and after it
      vkCmdPipelineBarrier(cmd,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           0, // dependency flags
                           0, nullptr, // memory barrier count and array
                           0, nullptr, // buffer memory barrier count and array
                           1, &imageBarrier_toTransfer); // image memory barrier count and array

      copyRegion.bufferOffset = stagingBufferOffset;
      copyRegion.imageExtent = imageExtent;

      //copy the buffer into the image
      vkCmdCopyBufferToImage(cmd, stagingVMABuffer.vkBuffer, allocImage.vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

      // barrier the image into the shader readable layout
      imageBarrier_toReadable.image = allocImage.vkImage;
      vkCmdPipelineBarrier(cmd,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &imageBarrier_toReadable);

      stagingBufferOffset += textureInfo.textureSize;
    }

  });

  vmaDestroyBuffer(vmaAllocator, stagingVMABuffer.vkBuffer, stagingVMABuffer.vmaAllocation);
}