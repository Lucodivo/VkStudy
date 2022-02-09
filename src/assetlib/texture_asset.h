#pragma once

#include "asset_loader.h"

namespace assets {
  enum class TextureFormat : u32
  {
    Unknown = 0,
#define TextureFormat(name) name,
#include "texture_format.incl"
#undef TextureFormat
  };

  // TODO: use pages for mipmapping
//  struct TexturePageInfo {
//    u32 width;
//    u32 height;
//    u32 compressedSize;
//    s32 originalSize;
//  };

  struct TextureInfo {
    // Note: supplied by caller
    u64 textureSize;
    TextureFormat textureFormat;
    u32 width;
    u32 height;
    std::string originalFile;
    // Note: Filled in when packed
    CompressionMode compressionMode;
    u64 compressedSize;
    //std::vector<TexturePageInfo> pages; // TODO: Mipmapping
  };

  //parses the texture metadata from an asset file
  TextureInfo readTextureInfo(AssetFile* file);

  void unpackTexture(TextureInfo* info, const char* sourceBuffer, size_t sourceSize, char* destination);
  // void unpackTexturePage(TextureInfo* info, int pageIndex ,char* sourcebuffer, char* destination); // TODO: Mipmaps
  AssetFile packTexture(TextureInfo* info, void* pixelData);
}
