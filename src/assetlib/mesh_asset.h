#pragma once

#include "asset_loader.h"

namespace assets {
	struct Vertex_PNCV_f32 {
		f32 position[3];
    f32 normal[3];
    f32 color[3];
    f32 uv[2];
	};

	struct Vertex_P32N8C8V16 {
		float position[3];
		u8 normal[3];
		u8 color[3];
		float uv[2];
	};

  enum class VertexFormat : u32 {
    Unknown = 0,
#define VertexFormat(name) name,
#include "vertex_format.incl"
#undef VertexFormat
  };

	struct MeshBounds {
		float origin[3];
		float radius;
		float extents[3];
	};

	struct MeshInfo {
		u64 vertexBufferSize;
    u64 indexBufferSize;
		MeshBounds bounds;
		VertexFormat vertexFormat;
		u8 indexSize;
		CompressionMode compressionMode;
		std::string originalFile;
	};

	MeshInfo readMeshInfo(AssetFile* file);
	void unpackMesh(MeshInfo* info, const char* srcBuffer, size_t sourceSize, char* dstVertexBuffer, char* dstIndexBuffer);
	AssetFile packMesh(MeshInfo* info, char* vertexData, char* indexData);
	MeshBounds calculateBounds(Vertex_PNCV_f32* vertices, size_t vertexCount);
}