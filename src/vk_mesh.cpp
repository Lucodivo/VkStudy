#include <vk_mesh.h>
#include <tiny_obj_loader.h>
#include <iostream>

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

	//Color at Location 2
	VkVertexInputAttributeDescription colorAttribute = {};
	colorAttribute.binding = 0;
	colorAttribute.location = 2;
	colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
	colorAttribute.offset = offsetof(Vertex, color);

	description.bindings.push_back(mainBinding);
	description.attributes.push_back(positionAttribute);
	description.attributes.push_back(normalAttribute);
	description.attributes.push_back(colorAttribute);
	return description;
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

				//vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				//vertex normal
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				//copy it into our vertex
				Vertex newVert;
				newVert.position.x = vx;
				newVert.position.y = vy;
				newVert.position.z = vz;

				newVert.normal.x = nx;
				newVert.normal.y = ny;
				newVert.normal.z = nz;

				//we are setting the vertex color as the vertex normal. This is just for display purposes
				newVert.color = newVert.normal;

				vertices.push_back(newVert);
			}
			index_offset += fv;
		}
	}

	return true;
}
