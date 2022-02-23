VertexInputDescription Vertex::getVertexDescriptions() {
  VertexInputDescription description;

  // 1 vertex buffer binding, with a per-vertex rate
  VkVertexInputBindingDescription bindingDesc = {};
  bindingDesc.binding = 0; // this binding number connects the attributes to their binding description
  bindingDesc.stride = sizeof(Vertex);
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

  //Color at Location 2
  VkVertexInputAttributeDescription colorAttribute = {};
  colorAttribute.binding = 0;
  colorAttribute.location = 2;
  colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
  colorAttribute.offset = offsetof(Vertex, color);

  //Color at Location 3
  VkVertexInputAttributeDescription uvAttribute = {};
  uvAttribute.binding = 0;
  uvAttribute.location = 3;
  uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
  uvAttribute.offset = offsetof(Vertex, uv);

  description.bindingDesc = bindingDesc;
  description.attributes.push_back(positionAttribute);
  description.attributes.push_back(normalAttribute);
  description.attributes.push_back(colorAttribute);
  description.attributes.push_back(uvAttribute);
  return description;
}

bool Mesh::loadFromAsset(const char* fileName) {
  assets::AssetFile assetFile{};
  assets::loadAssetFile(fileName, &assetFile);

  assets::MeshInfo meshInfo{};
  assets::readMeshInfo(assetFile, &meshInfo);

  std::vector<char> tmpVertexBuffer;
  tmpVertexBuffer.resize(meshInfo.vertexBufferSize);

  indices.resize(meshInfo.indexBufferSize / sizeof(u32));

  assets::unpackMesh(meshInfo, assetFile.binaryBlob.data(), assetFile.binaryBlob.size(), tmpVertexBuffer.data(), (char*)indices.data());

  bounds.extents.x = meshInfo.bounds.extents[0];
  bounds.extents.y = meshInfo.bounds.extents[1];
  bounds.extents.z = meshInfo.bounds.extents[2];

  bounds.origin.x = meshInfo.bounds.origin[0];
  bounds.origin.y = meshInfo.bounds.origin[1];
  bounds.origin.z = meshInfo.bounds.origin[2];

  bounds.radius = meshInfo.bounds.radius;
  bounds.valid = true;

  vertices.clear();

  if(meshInfo.vertexFormat == assets::VertexFormat::PNCV_F32) {
    assets::Vertex_PNCV_f32* unpackedVertices = (assets::Vertex_PNCV_f32*)tmpVertexBuffer.data();
    u64 vertexCount = meshInfo.vertexBufferSize / sizeof(assets::Vertex_PNCV_f32);
    vertices.resize(vertexCount);

    for (u32 i = 0; i < vertexCount; i++) {
      const assets::Vertex_PNCV_f32& assetVertex = unpackedVertices[i];
      Vertex& newVertex = vertices[i];

      newVertex.position = {
              unpackedVertices[i].position[0],
              unpackedVertices[i].position[1],
              unpackedVertices[i].position[2]
      };

      newVertex.normal = {
              unpackedVertices[i].normal[0],
              unpackedVertices[i].normal[1],
              unpackedVertices[i].normal[2]
      };

      newVertex.color = {
              unpackedVertices[i].color[0],
              unpackedVertices[i].color[1],
              unpackedVertices[i].color[2]
      };

      newVertex.uv = {
              unpackedVertices[i].uv[0],
              unpackedVertices[i].uv[1]
      };
    }
  } else if(meshInfo.vertexFormat == assets::VertexFormat::P32N8C8V16) {
    assets::Vertex_P32N8C8V16* unpackedVertices = (assets::Vertex_P32N8C8V16*)tmpVertexBuffer.data();
    vertices.resize(meshInfo.vertexBufferSize / sizeof(assets::Vertex_P32N8C8V16));

    for (int i = 0; i < vertices.size(); i++) {
      const assets::Vertex_P32N8C8V16& assetVertex = unpackedVertices[i];
      Vertex& newVertex = vertices[i];

      newVertex.position = {
              unpackedVertices[i].position[0],
              unpackedVertices[i].position[1],
              unpackedVertices[i].position[2]
      };

      newVertex.normal = {
              (f32)unpackedVertices[i].normal[0],
              (f32)unpackedVertices[i].normal[1],
              (f32)unpackedVertices[i].normal[2]
      };

      newVertex.color = {
              (f32)unpackedVertices[i].color[0],
              (f32)unpackedVertices[i].color[1],
              (f32)unpackedVertices[i].color[2]
      };

      newVertex.uv = {
              unpackedVertices[i].uv[0],
              unpackedVertices[i].uv[1]
      };
    }
  }

  return false;
}

void Mesh::uploadMesh(VmaAllocator vmaAllocator, UploadContext& uploadContext) {
  u64 vertexBufferSize = vertices.size() * sizeof(Vertex);
  u64 indexBufferSize = indices.size() * sizeof(u32);

  // vertex buffer
  VkBufferCreateInfo stagingBufferCreateInfo = {};
  stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferCreateInfo.pNext = NULL;
  stagingBufferCreateInfo.size = vertexBufferSize + indexBufferSize;
  stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; // TODO: VMA_MEMORY_USAGE_CPU_TO_GPU

  AllocatedBuffer stagingBuffer;
  VK_CHECK(vmaCreateBuffer(vmaAllocator,
                           &stagingBufferCreateInfo,
                           &vmaAllocInfo,
                           &stagingBuffer.vkBuffer,
                           &stagingBuffer.vmaAllocation,
                           nullptr));

  {
    char* data;
    vmaMapMemory(vmaAllocator, stagingBuffer.vmaAllocation, (void**)(&data));
      memcpy(data, vertices.data(), vertexBufferSize);
      memcpy(data + vertexBufferSize, indices.data(), indexBufferSize);
    vmaUnmapMemory(vmaAllocator, stagingBuffer.vmaAllocation);
  }

  VkBufferCreateInfo vertexBufferCreateInfo = {};
  vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferCreateInfo.pNext = nullptr;
  vertexBufferCreateInfo.size = vertexBufferSize;
  vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  VkBufferCreateInfo indexBufferCreateInfo = vertexBufferCreateInfo;
  indexBufferCreateInfo.size = indexBufferSize;
  indexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

  VmaAllocationCreateInfo vmaStagingAllocInfo = {};
  vmaStagingAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VK_CHECK(vmaCreateBuffer(vmaAllocator, &vertexBufferCreateInfo, &vmaStagingAllocInfo,
                           &vertexBuffer.vkBuffer,
                           &vertexBuffer.vmaAllocation,
                           nullptr));

  VK_CHECK(vmaCreateBuffer(vmaAllocator, &indexBufferCreateInfo, &vmaStagingAllocInfo,
                           &indexBuffer.vkBuffer,
                           &indexBuffer.vmaAllocation,
                           nullptr));

  vkutil::immediateSubmit(uploadContext, [=](VkCommandBuffer cmd) -> void {
    VkBufferCopy vertexCopy;
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size = vertexBufferSize;

    VkBufferCopy indexCopy;
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vertexBufferSize;
    indexCopy.size = indexBufferSize;

    vkCmdCopyBuffer(cmd, stagingBuffer.vkBuffer, vertexBuffer.vkBuffer, 1, &vertexCopy);
    vkCmdCopyBuffer(cmd, stagingBuffer.vkBuffer, indexBuffer.vkBuffer, 1, &indexCopy);
  });

  vmaDestroyBuffer(vmaAllocator, stagingBuffer.vkBuffer, stagingBuffer.vmaAllocation);
}