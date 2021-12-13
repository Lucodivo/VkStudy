#include <vk_mesh.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>
#include <iostream>

#include "types.h"

VertexInputDescription Vertex::getVertexDescriptions()
{
	VertexInputDescription description;

	// 1 vertex buffer binding, with a per-vertex rate
	VkVertexInputBindingDescription mainBinding = {};
	mainBinding.binding = 0; // this binding number connects the attributes to their binding description
	mainBinding.stride = sizeof(Vertex);
	mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	//Position at Location 0
	VkVertexInputAttributeDescription positionAttribute = {};
	positionAttribute.binding = 0;
	positionAttribute.location = 0;
	positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	positionAttribute.offset = offsetof(Vertex, position);

	//Normal at Location 1
	VkVertexInputAttributeDescription normalAttribute = {};
	normalAttribute.binding = 0;
	normalAttribute.location = 1;
	normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	normalAttribute.offset = offsetof(Vertex, normal);

	description.bindings.push_back(mainBinding);
	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	return description;
}

bool Mesh::loadFromGltf(const char* file) {
	// simple check for .glb binary file type
	bool glb = false;
	size_t strLength = strlen(file);
	char lastChar = file[strLength - 1];
	if (lastChar == 'b' || lastChar == 'B') {
		glb = true;
	}
	
	struct gltfAttributeMetadata {
		u32 accessorIndex;
		u32 numComponents;
		u32 bufferViewIndex;
		u32 bufferIndex;
		u64 bufferByteOffset;
		u64 bufferByteLength;
	};

	const char* positionIndexKeyString = "POSITION";
	const char* normalIndexKeyString = "NORMAL";
	const char* texture0IndexKeyString = "TEXCOORD_0";
	const u32 positionAttributeIndex = 0;
	const u32 normalAttributeIndex = 1;
	const u32 texture0AttributeIndex = 2;

	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret;
	if (!glb) {
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, file);
	}
	else {
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, file);
	}


	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return -1;
	}

	auto populateAttributeMetadata = [](tinygltf::Model& model, const char* keyString, const tinygltf::Primitive* gltfPrimitive) -> gltfAttributeMetadata {
		gltfAttributeMetadata result;
		result.accessorIndex = gltfPrimitive->attributes.at(keyString);
		result.numComponents = tinygltf::GetNumComponentsInType(model.accessors[result.accessorIndex].type);
		result.bufferViewIndex = model.accessors[result.accessorIndex].bufferView;
		result.bufferIndex = model.bufferViews[result.bufferViewIndex].buffer;
		result.bufferByteOffset = model.bufferViews[result.bufferViewIndex].byteOffset;
		result.bufferByteLength = model.bufferViews[result.bufferViewIndex].byteLength;
		return result;
	};

	Assert(!gltfModel.meshes.empty());
	u32 meshCount = gltfModel.meshes.size();
	std::vector<tinygltf::Accessor>* gltfAccessors = &gltfModel.accessors;
	std::vector<tinygltf::BufferView>* gltfBufferViews = &gltfModel.bufferViews;
	for (u32 i = 0; i < meshCount; i++) {
		tinygltf::Mesh* gltfMesh = &gltfModel.meshes[i];
		Assert(!gltfMesh->primitives.empty());
		u32 primitiveCount = gltfMesh->primitives.size();
		for (u32 j = 0; j < primitiveCount; j++) {
			tinygltf::Primitive* gltfPrimitive = &gltfMesh->primitives[j];
			Assert(gltfPrimitive->indices > -1);
			
			// position attributes
			Assert(gltfPrimitive->attributes.find(positionIndexKeyString) != gltfPrimitive->attributes.end());
			gltfAttributeMetadata positionAttribute = populateAttributeMetadata(gltfModel, positionIndexKeyString, gltfPrimitive);
			Assert(positionAttribute.numComponents == 3);

			// normal attributes
			bool normalAttributesAvailable = gltfPrimitive->attributes.find(normalIndexKeyString) != gltfPrimitive->attributes.end();
			Assert(normalAttributesAvailable)
			gltfAttributeMetadata normalAttribute{};
			normalAttribute = populateAttributeMetadata(gltfModel, normalIndexKeyString, gltfPrimitive);
			Assert(positionAttribute.bufferIndex == normalAttribute.bufferIndex);
			Assert(normalAttribute.numComponents == 3);

			// TODO: texture 0 uv coord attribute data
			//bool texture0AttributesAvailable = gltfPrimitive->attributes.find(texture0IndexKeyString) != gltfPrimitive->attributes.end();
			//gltfAttributeMetadata texture0Attribute{};
			//if (texture0AttributesAvailable) { 
			//	texture0Attribute = populateAttributeMetadata(gltfModel, texture0IndexKeyString, gltfPrimitive);
			//	Assert(positionAttribute.bufferIndex == texture0Attribute.bufferIndex);
			//}

			u32 indicesAccessorIndex = gltfPrimitive->indices;
			tinygltf::BufferView indicesGLTFBufferView = gltfBufferViews->at(gltfAccessors->at(indicesAccessorIndex).bufferView);
			u32 indicesGLTFBufferIndex = indicesGLTFBufferView.buffer;
			u64 indicesGLTFBufferByteOffset = indicesGLTFBufferView.byteOffset;
			u64 indicesGLTFBufferByteLength = indicesGLTFBufferView.byteLength;
			u16* indicesDataOffset = (u16*)(gltfModel.buffers[indicesGLTFBufferIndex].data.data() + indicesGLTFBufferByteOffset);

			Assert(gltfModel.buffers.size() > positionAttribute.bufferIndex);
			f32* positionAttributeData = (f32*)(gltfModel.buffers[positionAttribute.bufferIndex].data.data() + positionAttribute.bufferByteOffset);
			f32* normalAttributeData = (f32*)(gltfModel.buffers[normalAttribute.bufferIndex].data.data() + normalAttribute.bufferByteOffset);
			Assert(positionAttribute.bufferByteLength == normalAttribute.bufferByteLength);

			Vertex vertex{};
			
			u32 indexCount = indicesGLTFBufferByteLength / sizeof(u16);
			u32 vertexCount = positionAttribute.bufferByteLength / positionAttribute.numComponents / sizeof(f32);
			for (u32 i = 0; i < indexCount; i++) {
				u16 vertIndex = indicesDataOffset[i];

				Vertex newVert;

				//vertex position
				newVert.position.x = positionAttributeData[3 * vertIndex + 0];
				newVert.position.y = positionAttributeData[3 * vertIndex + 1];
				newVert.position.z = positionAttributeData[3 * vertIndex + 2];

				//vertex normal
				newVert.normal.x = normalAttributeData[3 * vertIndex + 0];
				newVert.normal.y = normalAttributeData[3 * vertIndex + 1];
				newVert.normal.z = normalAttributeData[3 * vertIndex + 2];

				vertices.push_back(newVert);
			}
		}
	}
}

bool Mesh::loadFromObj(const char* file, const char* materialDir)
{
	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.mtl_search_path = materialDir;

	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(file, readerConfig)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "WARN (tinyobjloader): " << reader.Warning();
	}


	//attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib = reader.GetAttrib();
	//shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
	//materials contains the information about the material of each shape, but we won't use it.
	std::vector<tinyobj::material_t> materials = reader.GetMaterials();

	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			//hardcode loading to triangles
			int fv = 3;
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				Vertex newVert;

				//vertex position
				newVert.position.x = attrib.vertices[3 * idx.vertex_index + 0];
				newVert.position.y = attrib.vertices[3 * idx.vertex_index + 1];
				newVert.position.z = attrib.vertices[3 * idx.vertex_index + 2];

				//vertex normal
				newVert.normal.x = attrib.normals[3 * idx.normal_index + 0];
				newVert.normal.y = attrib.normals[3 * idx.normal_index + 1];
				newVert.normal.z = attrib.normals[3 * idx.normal_index + 2];

				vertices.push_back(newVert);
			}
			index_offset += fv;
		}
	}

	return true;
}

void Mesh::uploadMesh(VmaAllocator& allocator, DeletionQueue& deletionQueue)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = vertices.size() * sizeof(Vertex);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
		&vertexBuffer.buffer,
		&vertexBuffer.allocation,
		nullptr));

	void* data;
	vmaMapMemory(allocator, vertexBuffer.allocation, &data);
		memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(allocator, vertexBuffer.allocation);

	deletionQueue.pushFunction([allocator, vertexBuffer = this->vertexBuffer]() {
		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
	});
}
