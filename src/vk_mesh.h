#pragma once

struct VertexInputDescription {
  VkVertexInputBindingDescription bindingDesc;
  std::vector<VkVertexInputAttributeDescription> attributes;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 uv;

  static VertexInputDescription getVertexDescriptions();
};

struct Mesh {
  std::vector<Vertex> vertices;
  AllocatedBuffer vertexBuffer;

  bool loadFromFile(const char* fileName);
  bool loadFromGltf(const char* fileName);
  bool loadFromObj(const char* filename);
  AllocatedBuffer uploadMesh(VmaAllocator vmaAllocator);
};