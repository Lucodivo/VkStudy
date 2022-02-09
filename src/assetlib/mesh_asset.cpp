#include "mesh_asset.h"

#include "json.hpp"
#include "lz4.h"

const internal_access char* mapVertexFormatToString[] = {
        "Unknown",
#define VertexFormat(name) #name,
#include "vertex_format.incl"
#undef VertexFormat
};

const internal_access char* MESH_FOURCC = "MESH";

const struct {
  const char* vertexFormat = "vertex_format";
  const char* vertexFormatEnumVal = "vertex_format_enum_val";
  const char* vertexBufferSize = "vertex_buffer_size";
  const char* indexBufferSize = "index_buffer_size";
  const char* indexSize = "index_size";
  const char* originalFile = "original_file";
  const char* bounds = "bound";
  const char* compressionMode = "compression_mode";
  const char* compressionModeEnumVal = "compression_mode_enum_val";
} jsonKeys;

const char* vertexFormatToString(assets::VertexFormat format);
u32 vertexFormatToEnumVal(assets::VertexFormat format);

assets::MeshInfo assets::readMeshInfo(AssetFile* file)
{
	MeshInfo info;

	nlohmann::json meshJson = nlohmann::json::parse(file->json);

	info.vertexBufferSize = meshJson[jsonKeys.vertexBufferSize];
	info.indexBufferSize = meshJson[jsonKeys.indexBufferSize];
	info.indexSize = static_cast<u8>(meshJson[jsonKeys.indexSize]);
	info.originalFile = meshJson[jsonKeys.originalFile];

	std::string compressionString = meshJson[jsonKeys.compressionMode];
  u32 compressionModeEnumVal = meshJson[jsonKeys.compressionModeEnumVal];
	info.compressionMode = CompressionMode(compressionModeEnumVal);

	std::vector<f32> boundsData;
	boundsData.reserve(7);
	boundsData = meshJson[jsonKeys.bounds].get<std::vector<f32>>();

	info.bounds.origin[0] = boundsData[0];
	info.bounds.origin[1] = boundsData[1];
	info.bounds.origin[2] = boundsData[2];
		
	info.bounds.radius = boundsData[3];
	
	info.bounds.extents[0] = boundsData[4];
	info.bounds.extents[1] = boundsData[5];
	info.bounds.extents[2] = boundsData[6];

	std::string vertexFormat = meshJson[jsonKeys.vertexFormat];
  u32 vertexFormatEnumVal = meshJson[jsonKeys.vertexFormatEnumVal];
	info.vertexFormat = VertexFormat(vertexFormatEnumVal);

  return info;
}

void assets::unpackMesh(MeshInfo* info, const char* srcBuffer, size_t sourceSize, char* dstVertexBuffer, char* dstIndexBuffer)
{
	std::vector<char> decompressedBuffer;
  const u64 decompressedBufferSize = info->vertexBufferSize + info->indexBufferSize;
	decompressedBuffer.resize(decompressedBufferSize);

	LZ4_decompress_safe(srcBuffer, decompressedBuffer.data(), (s32)sourceSize, (s32)decompressedBufferSize);

	//copy vertex buffer
	memcpy(dstVertexBuffer, decompressedBuffer.data(), info->vertexBufferSize);

	//copy index buffer
	memcpy(dstIndexBuffer, decompressedBuffer.data() + info->vertexBufferSize, info->indexBufferSize);
}

assets::AssetFile assets::packMesh(MeshInfo* info, char* vertexData, char* indexData)
{
  AssetFile file;
  strncpy(file.type, MESH_FOURCC, 4);
	file.version = ASSET_LIB_VERSION;

	nlohmann::json meshJson;
  meshJson[jsonKeys.vertexFormat] = vertexFormatToString(info->vertexFormat);
  meshJson[jsonKeys.vertexFormatEnumVal] = vertexFormatToEnumVal(info->vertexFormat);
  meshJson[jsonKeys.vertexBufferSize] = info->vertexBufferSize;
  meshJson[jsonKeys.indexBufferSize] = info->indexBufferSize;
  meshJson[jsonKeys.indexSize] = info->indexSize;
  meshJson[jsonKeys.originalFile] = info->originalFile;

	std::vector<float> boundsData;
	boundsData.resize(7);

	boundsData[0] = info->bounds.origin[0];
	boundsData[1] = info->bounds.origin[1];
	boundsData[2] = info->bounds.origin[2];

	boundsData[3] = info->bounds.radius;

	boundsData[4] = info->bounds.extents[0];
	boundsData[5] = info->bounds.extents[1];
	boundsData[6] = info->bounds.extents[2];

  meshJson[jsonKeys.bounds] = boundsData;

	size_t fullSize = info->vertexBufferSize + info->indexBufferSize;

	std::vector<char> mergedBuffer;
	mergedBuffer.resize(fullSize);

	//copy vertex buffer
	memcpy(mergedBuffer.data(), vertexData, info->vertexBufferSize);

	//copy index buffer
	memcpy(mergedBuffer.data() + info->vertexBufferSize, indexData, info->indexBufferSize);

	//compress buffer and copy it into the file struct
	size_t worstCaseCompressionSize = LZ4_compressBound(static_cast<int>(fullSize));
	file.binaryBlob.resize(worstCaseCompressionSize);
	int actualCompressionSize = LZ4_compress_default(mergedBuffer.data(), file.binaryBlob.data(), static_cast<int>(mergedBuffer.size()), static_cast<int>(worstCaseCompressionSize));
	file.binaryBlob.resize(actualCompressionSize);

  meshJson[jsonKeys.compressionMode] = compressionModeToString(CompressionMode::LZ4);
  meshJson[jsonKeys.compressionModeEnumVal] = compressionModeToEnumVal(CompressionMode::LZ4);

	file.json = meshJson.dump();

	return file;
}

assets::MeshBounds assets::calculateBounds(Vertex_PNCV_f32* vertices, size_t vertexCount)
{
	MeshBounds bounds{};

  f32 min[3] = { std::numeric_limits<f32>::max(),std::numeric_limits<f32>::max(),std::numeric_limits<f32>::max() };
  f32 max[3] = { std::numeric_limits<f32>::min(),std::numeric_limits<f32>::min(),std::numeric_limits<f32>::min() };

	for (int i = 0; i < vertexCount; i++) {
		min[0] = std::min(min[0], vertices[i].position[0]);
		min[1] = std::min(min[1], vertices[i].position[1]);
		min[2] = std::min(min[2], vertices[i].position[2]);

		max[0] = std::max(max[0], vertices[i].position[0]);
		max[1] = std::max(max[1], vertices[i].position[1]);
		max[2] = std::max(max[2], vertices[i].position[2]);
	}

	bounds.extents[0] = (max[0] - min[0]) / 2.0f;
	bounds.extents[1] = (max[1] - min[1]) / 2.0f;
	bounds.extents[2] = (max[2] - min[2]) / 2.0f;

	bounds.origin[0] = bounds.extents[0] + min[0];
	bounds.origin[1] = bounds.extents[1] + min[1];
	bounds.origin[2] = bounds.extents[2] + min[2];

	// exact bounding sphere radius
	float radSq = 0;
	for (int i = 0; i < vertexCount; i++) {
		float offset[3];
		offset[0] = vertices[i].position[0] - bounds.origin[0];
		offset[1] = vertices[i].position[1] - bounds.origin[1];
		offset[2] = vertices[i].position[2] - bounds.origin[2];

		float distance = (offset[0] * offset[0]) + (offset[1] * offset[1]) + (offset[2] * offset[2]);
    radSq = std::max(radSq, distance);
	}

	bounds.radius = std::sqrt(radSq);

	return bounds;
}

const char* vertexFormatToString(assets::VertexFormat format) {
  return mapVertexFormatToString[vertexFormatToEnumVal(format)];
}

inline u32 vertexFormatToEnumVal(assets::VertexFormat format) {
  return static_cast<u32>(format);
}