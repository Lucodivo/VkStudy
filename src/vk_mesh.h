#pragma once

struct VertexInputDescription {
  VkVertexInputBindingDescription bindingDesc;
  std::vector<VkVertexInputAttributeDescription> attributes;
};

struct Vertex {
  vec3 position;
  vec3 normal;
  vec3 color;
  vec2 uv;

  static VertexInputDescription getVertexDescriptions();
};

struct RenderBounds {
  vec3 origin;
  float radius;
  vec3 extents;
  bool valid;
};

struct Mesh {
  std::vector<Vertex> vertices;
  AllocatedBuffer vertexBuffer;
  // TODO: Index buffer
  RenderBounds bounds;

  bool loadFromAsset(const char* fileName);
  AllocatedBuffer uploadMesh(VmaAllocator vmaAllocator);
};