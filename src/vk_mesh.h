#pragma once

#include <vk_types.h>
#include <vector>
#include <glm/vec3.hpp>

struct VertexInputDescription {

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;

    static VertexInputDescription getVertexDescriptions();
};

struct Mesh {
    std::vector<Vertex> vertices;
    AllocatedBuffer vertexBuffer;

    bool loadFromGltf(const char* file);
    bool loadFromObj(const char* filename, const char* directory);
    void uploadMesh(VmaAllocator& allocator, DeletionQueue& deletionQueue);
};