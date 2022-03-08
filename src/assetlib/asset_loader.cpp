#include "asset_loader.h"

#include <fstream>

const internal_access char* mapCompressionModeToString[] = {
        "None",
#define CompressionMode(name) #name,
#include "compression_mode.incl"
#undef CompressionMode
};

bool assets::saveAssetFile(const char* path, const AssetFile& file) {
  std::ofstream outfile;
  outfile.open(path, std::ios::binary | std::ios::out);

  // file type
  outfile.write(file.type, FILE_TYPE_SIZE_IN_BYTES);

  // version
  u32 version = file.version;
  outfile.write((const char*)&version, sizeof(u32));

  // json length
  u32 jsonLength = static_cast<u32>(file.json.size());
  outfile.write((const char*)&jsonLength, sizeof(u32));

  // blob length
  u32 bloblength = static_cast<u32>(file.binaryBlob.size());
  outfile.write((const char*)&bloblength, sizeof(u32));

  //json
  outfile.write(file.json.data(), jsonLength);

  //blob data
  outfile.write(file.binaryBlob.data(), bloblength);

  outfile.close();

  return true;
}

bool assets::loadAssetFile(const char* path, AssetFile* outputFile) {
  std::ifstream infile;
  infile.open(path, std::ios::binary);

  if (!infile.is_open()) return false;

  //move file cursor to beginning
  infile.seekg(0);

  // file type
  infile.read(outputFile->type, FILE_TYPE_SIZE_IN_BYTES);

  // version
  infile.read((char*)&outputFile->version, sizeof(u32));
  if(outputFile->version != ASSET_LIB_VERSION) {
    printf("Attempting to load asset with version #%d. Asset Loader version is currently #%d.", outputFile->version, ASSET_LIB_VERSION);
  }

  // json length
  u32 jsonLength;
  infile.read((char*)&jsonLength, sizeof(u32));

  // blob length
  u32 blobLength;
  infile.read((char*)&blobLength, sizeof(u32));

  // json
  outputFile->json.resize(jsonLength);
  infile.read(outputFile->json.data(), jsonLength);

  // blob
  outputFile->binaryBlob.resize(blobLength);
  infile.read(outputFile->binaryBlob.data(), blobLength);

  return true;
}

const char* assets::compressionModeToString(CompressionMode compressionMode) {
  return mapCompressionModeToString[compressionModeToEnumVal(compressionMode)];
}

inline u32 assets::compressionModeToEnumVal(CompressionMode compressionMode) {
  return static_cast<u32>(compressionMode);
}