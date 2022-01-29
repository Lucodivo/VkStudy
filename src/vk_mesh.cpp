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

bool Mesh::loadFromFile(const char* fileName) {
  // dumb simple check for file type
  u64 strLength = strlen(fileName);
  char lastChar = fileName[strLength - 1];
  switch(lastChar) {
    case 'l':
    case 'L':
    case 'b':
    case 'B':
      return loadFromGltf(fileName);
    case 'j':
    case 'J':
      return loadFromObj(fileName);
    default:
      return false;
  }
}

bool Mesh::loadFromGltf(const char* fileName) {
  // simple check for .glb binary file type
  bool glb = false;
  u64 strLength = strlen(fileName);
  char lastChar = fileName[strLength - 1];
  if(lastChar == 'b' || lastChar == 'B') {
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
  if(!glb) {
    ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, fileName);
  } else {
    ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, fileName);
  }


  if(!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if(!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if(!ret) {
    printf("Failed to parse glTF\n");
    false;
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
  u64 meshCount = gltfModel.meshes.size();
  std::vector<tinygltf::Accessor>* gltfAccessors = &gltfModel.accessors;
  std::vector<tinygltf::BufferView>* gltfBufferViews = &gltfModel.bufferViews;
  for(u32 i = 0; i < meshCount; i++) {
    tinygltf::Mesh* gltfMesh = &gltfModel.meshes[i];
    Assert(!gltfMesh->primitives.empty());
    u64 primitiveCount = gltfMesh->primitives.size();
    for(u32 j = 0; j < primitiveCount; j++) {
      tinygltf::Primitive* gltfPrimitive = &gltfMesh->primitives[j];
      Assert(gltfPrimitive->indices > -1);

      // indices data
      u32 indicesAccessorIndex = gltfPrimitive->indices;
      tinygltf::BufferView indicesGLTFBufferView = gltfBufferViews->at(gltfAccessors->at(indicesAccessorIndex).bufferView);
      u32 indicesGLTFBufferIndex = indicesGLTFBufferView.buffer;
      u64 indicesGLTFBufferByteOffset = indicesGLTFBufferView.byteOffset;
      u64 indicesGLTFBufferByteLength = indicesGLTFBufferView.byteLength;
      u16* indicesDataOffset = (u16*)(gltfModel.buffers[indicesGLTFBufferIndex].data.data() + indicesGLTFBufferByteOffset);

      // position attributes
      Assert(gltfPrimitive->attributes.find(positionIndexKeyString) != gltfPrimitive->attributes.end());
      gltfAttributeMetadata positionAttribute = populateAttributeMetadata(gltfModel, positionIndexKeyString, gltfPrimitive);
      Assert(positionAttribute.numComponents == 3);

      // normal attributes
      bool normalAttributesAvailable = gltfPrimitive->attributes.find(normalIndexKeyString) != gltfPrimitive->attributes.end();
      Assert(normalAttributesAvailable);
      gltfAttributeMetadata normalAttribute{};
      normalAttribute = populateAttributeMetadata(gltfModel, normalIndexKeyString, gltfPrimitive);
      Assert(positionAttribute.bufferIndex == normalAttribute.bufferIndex);
      Assert(normalAttribute.numComponents == 3);

      // texture 0 uv coord attribute data
      bool texture0AttributesAvailable = gltfPrimitive->attributes.find(texture0IndexKeyString) != gltfPrimitive->attributes.end();
      gltfAttributeMetadata texture0Attribute{};
      f32* texture0AttributeData;
      if(texture0AttributesAvailable) {
        texture0Attribute = populateAttributeMetadata(gltfModel, texture0IndexKeyString, gltfPrimitive);
        texture0AttributeData = (f32*)(gltfModel.buffers[texture0Attribute.bufferIndex].data.data() + texture0Attribute.bufferByteOffset);
        Assert(positionAttribute.bufferIndex == texture0Attribute.bufferIndex);
      }

      Assert(gltfModel.buffers.size() > positionAttribute.bufferIndex);
      f32* positionAttributeData = (f32*)(gltfModel.buffers[positionAttribute.bufferIndex].data.data() + positionAttribute.bufferByteOffset);
      f32* normalAttributeData = (f32*)(gltfModel.buffers[normalAttribute.bufferIndex].data.data() + normalAttribute.bufferByteOffset);
      Assert(positionAttribute.bufferByteLength == normalAttribute.bufferByteLength);

      tinygltf::Material gltfMaterial = gltfModel.materials[gltfPrimitive->material];
      f64* baseColor = gltfMaterial.pbrMetallicRoughness.baseColorFactor.data();
      glm::vec3 color = {
              (f32)baseColor[0],
              (f32)baseColor[1],
              (f32)baseColor[2]
      };

      u64 indexCount = indicesGLTFBufferByteLength / sizeof(u16);
      u64 vertexCount = positionAttribute.bufferByteLength / positionAttribute.numComponents / sizeof(f32);
      for(u32 i = 0; i < indexCount; i++) {
        u16 vertIndex = indicesDataOffset[i];

        Vertex newVert{};

        //vertex position
        newVert.position.x = positionAttributeData[3 * vertIndex + 0];
        newVert.position.y = positionAttributeData[3 * vertIndex + 1];
        newVert.position.z = positionAttributeData[3 * vertIndex + 2];

        //vertex normal
        newVert.normal.x = normalAttributeData[3 * vertIndex + 0];
        newVert.normal.y = normalAttributeData[3 * vertIndex + 1];
        newVert.normal.z = normalAttributeData[3 * vertIndex + 2];

        //vertex color
        newVert.color = color;

        // vertex uv
        if(texture0AttributesAvailable) {
          newVert.uv.x = texture0AttributeData[3 * vertIndex + 0];
          newVert.uv.y = 1.0f - texture0AttributeData[3 * vertIndex + 1]; // TODO: is inverse uv y coord necessary?
        } else {
          newVert.uv.x = 0.5f;
          newVert.uv.y = 0.5f;
        }

        vertices.push_back(newVert);
      }
    }
  }

  return true;
}

bool Mesh::loadFromObj(const char* fileName) {
  // find directory of file
  u64 strLength = strlen(fileName);
  const char* dirSlash = fileName + (strLength - 1);
  while(dirSlash != fileName) {
    if(*dirSlash == '/') {
      break;
    }
    dirSlash--;
  }

  // copy directory to new str to use as material search path
  char materialSearchPath[256];
  const char* copyIter = fileName;
  u32 copyIndex = 0;
  while(copyIter != dirSlash) {
    materialSearchPath[copyIndex++] = *copyIter++;
  }
  if(dirSlash != fileName) { // we want empty string if there was no dir slash
    materialSearchPath[copyIndex++] = '/';
  }
  materialSearchPath[copyIndex] = '\0';

  tinyobj::ObjReaderConfig readerConfig;
  readerConfig.mtl_search_path = materialSearchPath;

  tinyobj::ObjReader reader;
  if(!reader.ParseFromFile(fileName, readerConfig)) {
    if(!reader.Error().empty()) {
      std::cerr << "TinyObjReader: " << reader.Error();
    }
    exit(1);
  }

  if(!reader.Warning().empty()) {
    std::cout << "WARN (tinyobjloader): " << reader.Warning();
  }


  //attrib will contain the vertex arrays of the file
  tinyobj::attrib_t attrib = reader.GetAttrib();
  //shapes contains the info for each separate object in the file
  std::vector<tinyobj::shape_t> shapes = reader.GetShapes();
  //materials contains the information about the material of each shape, but we won't use it.
  std::vector<tinyobj::material_t> materials = reader.GetMaterials();

  for(u32 s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    u32 index_offset = 0;
    for(u64 f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      //hardcode loading to triangles
      const u32 verticesPerTriangle = 3;
      // Loop over vertices in the face.
      for(u32 v = 0; v < verticesPerTriangle; v++) {
        // access to vertex
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

        Vertex newVert;

        //vertex position
        newVert.position.x = attrib.vertices[(3 * idx.vertex_index) + 0];
        newVert.position.y = attrib.vertices[(3 * idx.vertex_index) + 1];
        newVert.position.z = attrib.vertices[(3 * idx.vertex_index) + 2];

        //vertex normal
        newVert.normal.x = attrib.normals[(3 * idx.normal_index) + 0];
        newVert.normal.y = attrib.normals[(3 * idx.normal_index) + 1];
        newVert.normal.z = attrib.normals[(3 * idx.normal_index) + 2];

        // Optional: vertex colors
        if(!attrib.colors.empty()) {
          newVert.color.r = attrib.colors[(3 * idx.vertex_index) + 0];
          newVert.color.g = attrib.colors[(3 * idx.vertex_index) + 1];
          newVert.color.b = attrib.colors[(3 * idx.vertex_index) + 2];
        } else {
          newVert.color = newVert.normal;
        }

        //vertex uv
        newVert.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
        newVert.uv.y = 1 - attrib.texcoords[2 * idx.texcoord_index + 1]; // TODO: Is inverting uv y coordinate correct?

        vertices.push_back(newVert);
      }
      index_offset += verticesPerTriangle;
    }
  }

  return true;
}

AllocatedBuffer Mesh::uploadMesh(VmaAllocator allocator) {
  //allocate vertex buffer
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = vertices.size() * sizeof(Vertex);
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
                           &vertexBuffer.vkBuffer,
                           &vertexBuffer.vmaAllocation,
                           nullptr));

  void* data;
  vmaMapMemory(allocator, vertexBuffer.vmaAllocation, &data);
  memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(allocator, vertexBuffer.vmaAllocation);

  return vertexBuffer;
}
