#include <iostream>
#include <json.hpp>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include <lz4.h>
#include <chrono>

#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <tiny_gltf.h>

#include <asset_loader.h>
#include <texture_asset.h>
#include <mesh_asset.h>
#include <material_asset.h>
#include <prefab_asset.h>
using namespace assets;

#include <glm/glm.hpp>
#include<glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

// TODO: Do I care about supporting non-gltf scenes?
//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>

struct {
  const char* texture = ".tx";
  const char* mesh = ".mesh";
  const char* material = ".mat";
  const char* prefab = ".pfb";
} extensions;

struct ConverterState {
  fs::path assetsDir;
  fs::path bakedAssetDir;
  fs::path outputFileDir;
  std::vector<fs::path> bakedFilePaths;

  fs::path convertToExportRelative(const fs::path& path)const;
};

bool convertImage(const fs::path& inputPath, ConverterState& converterState);

void packVertex(assets::Vertex_PNCV_f32& new_vert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t ux, tinyobj::real_t uy);
void packVertex(assets::Vertex_P32N8C8V16& new_vert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t ux, tinyobj::real_t uy);

void unpackGltfBuffer(tinygltf::Model& model, tinygltf::Accessor& accessor, std::vector<u8> &outputBuffer);
void extractGltfVertices(tinygltf::Primitive& primitive, tinygltf::Model& model, std::vector<assets::Vertex_PNCV_f32>& _vertices);
void extractGltfIndices(tinygltf::Primitive& primitive, tinygltf::Model& model, std::vector<u32>& _primIndices);
bool extractGltfMeshes(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState);
void extractGltfMaterials(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState);
void extractGltfNodes(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState);
std::string calculateGltfMaterialName(tinygltf::Model& model, int materialIndex);
std::string calculateGltfMeshName(tinygltf::Model& model, int meshIndex, int primitiveIndex);

// TODO: Do I care about objs?
//bool convertObjMesh(const fs::path& input, const fs::path& output);
//template<typename V>
//void extractMeshFromObj(std::vector<tinyobj::shape_t>& shapes, tinyobj::attrib_t& attrib, std::vector<u32>& _indices, std::vector<V>& _vertices);

// TODO: Do I care about supporting non-gltf scenes?
//std::string calculateAssimpMeshName(const aiScene* scene, int meshIndex);
//std::string calculateAssimpMaterialName(const aiScene* scene, int materialIndex);
//void extractAssimpMaterials(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState);
//void extractAssimpMeshes(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState);
//void extractAssimpNodes(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState);

void writeOutputData(const ConverterState& converterState);
std::size_t fileCountInDir(fs::path dirPath);

int main(int argc, char* argv[]) {

  // NOTE: Count is often at least 1, as argv[0] is full path of the program being run
  if (argc < 3)
  {
    std::cout << "You need to supply the assets directory";
    return -1;
  }

  ConverterState converterState;
  converterState.assetsDir = { argv[1] };
  converterState.bakedAssetDir = converterState.assetsDir.parent_path()/"assets_export";
  converterState.outputFileDir = { argv[2] };

  if (!fs::is_directory(converterState.assetsDir)) {
    std::cout << "Invalid assets directory: " << argv[1];
    return -1;
  }

  // Create export folder if needed
  if (!fs::is_directory(converterState.bakedAssetDir))
  {
    fs::create_directory(converterState.bakedAssetDir);
  }

  std::cout << "loaded asset directory at " << converterState.assetsDir << std::endl;

  size_t fileCount = fileCountInDir(converterState.assetsDir);
  converterState.bakedFilePaths.reserve(fileCount * 4);
  for (const fs::directory_entry& p : fs::directory_iterator(converterState.assetsDir)) { //fs::recursive_directory_iterator(directory)) {
    std::cout << "File: " << p << std::endl;

    // skip any directory
    if (fs::is_directory(p.path())) { continue; }

    if (p.path().extension() == ".png" || p.path().extension() == ".jpg" || p.path().extension() == ".TGA")
    {
    	std::cout << "found a texture" << std::endl;

    	convertImage(p.path(), converterState);
    }

    //if (p.path().extension() == ".obj") {
    //	std::cout << "found a mesh" << std::endl;
    //
    //	bakedAssetDir.replace_extension(extensions.mesh);
    //	convertObjMesh(p.path(), bakedAssetDir);
    //}

    bool gltfExtension = p.path().extension() == ".gltf";
    bool glbExtension = p.path().extension() == ".glb";
    if (gltfExtension || glbExtension) {
      using namespace tinygltf;
      Model model;
      TinyGLTF loader;
      std::string err;
      std::string warn;

      bool ret;
      if(gltfExtension) {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, p.path().string().c_str());
      } else { // glbExtension
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, p.path().string().c_str());
      }

      if(!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
      }

      if(!err.empty()) {
        printf("Err: %s\n", err.c_str());
      }

      if(!ret) {
        printf("Failed to parse glTF\n");
        return -1;
      } else {
        fs::path folder = converterState.bakedAssetDir/(p.path().stem().string() + "_GLTF");
        fs::create_directory(folder);

        extractGltfMeshes(model, p.path(), folder, converterState);
        extractGltfMaterials(model, p.path(), folder, converterState);
        extractGltfNodes(model, p.path(), folder, converterState);
      }
    }

//      if (p.path().extension() == ".fbx") {
//      const aiScene* scene;
//      Assimp::Importer importer;
//      //ZoneScopedNC("Assimp load", tracy::Color::Magenta);
//      const char* path = p.path().string().c_str();
//      auto start1 = std::chrono::system_clock::now();
//      scene = importer.ReadFile(p.path().string(), aiProcess_OptimizeMeshes | aiProcess_GenNormals | aiProcess_FlipUVs); //aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);
//      auto end = std::chrono::system_clock::now();
//      auto elapsed = end - start1;
//      std::cout << "Assimp load time " << elapsed.count() << '\n';
//      auto folder = bakedAssetDir.parent_path()/(p.path().stem().string() + "_GLTF");
//      fs::create_directory(folder);
//      extractAssimpMaterials(scene, p.path(), folder, converterState);
//      extractAssimpMeshes(scene, p.path(), folder, converterState);
//      extractAssimpNodes(scene, p.path(), folder, converterState);
//
//      std::vector<aiMaterial*> materials;
//      std::vector<std::string> materialNames;
//      materials.reserve(scene->mNumMaterials);
//      for (int m = 0; m < scene->mNumMaterials; m++) {
//        materials.push_back(scene->mMaterials[m]);
//        materialNames.push_back(scene->mMaterials[m]->GetName().C_Str());
//      }
//
//      std::cout << importer.GetErrorString();
//
//      std::cout << "Assimp Meshes: " << scene->mMeshes;
//    }
  }

  writeOutputData(converterState);

  return 0;
}

void replace(std::string& str, char oldToken, char newToken) {
  for(char& c : str) {
    if(c == oldToken) {
      c = newToken;
    }
  }
}

void addEscapeSlashes(std::string& str) {
  u32 originalStrLength = (u32)str.size();

  std::vector<u32> escapeSlashIndices;
  escapeSlashIndices.reserve(originalStrLength / 2);

  u32 slashCount = 0;
  for(u32 i = 0; i < originalStrLength; i++) {
    char& c = str[i];
    if(c == '\\') {
      escapeSlashIndices.push_back(i);
      slashCount++;
    }
  }

  u32 newStrLength = originalStrLength + slashCount;
  str.resize(newStrLength);

  u32 newStrIter = newStrLength - 1;
  u32 oldStrIter = originalStrLength - 1;
  while(slashCount > 0) {
    if(str[oldStrIter] == '\\') {
      str[newStrIter--] = '\\';
      slashCount--;
    }
    str[newStrIter--] = str[oldStrIter--];
  }
}

void writeOutputData(const ConverterState& converterState) {
  if(!fs::is_directory(converterState.outputFileDir)) {
    fs::create_directory(converterState.outputFileDir);
  }

  std::ofstream outTexturesFile, outMeshFile, outMaterialFile, outPrefabFile;
  outTexturesFile.open((converterState.outputFileDir / "baked_textures.incl").string(), std::ios::out);
  outMeshFile.open((converterState.outputFileDir / "baked_meshes.incl").string(), std::ios::out);
  outMaterialFile.open((converterState.outputFileDir / "baked_materials.incl").string(), std::ios::out);
  outPrefabFile.open((converterState.outputFileDir / "baked_prefabs.incl").string(), std::ios::out);

  for(const fs::path& path : converterState.bakedFilePaths) {
    std::string extensionStr = path.extension().string();
    const char* extension = extensionStr.c_str();
    std::string fileName = path.filename().replace_extension("").string();
    replace(fileName, '.', '_');
    std::string filePath = path.string();
    addEscapeSlashes(filePath);
    if(strcmp(extension, extensions.texture) == 0) {
      outTexturesFile << "BakedTexture(" << fileName << ",\"" << filePath << "\")\n";
    } else if(strcmp(extension, extensions.material) == 0) {
      outMaterialFile << "BakedMaterial(" << fileName << ",\"" << filePath << "\")\n";
    } else if(strcmp(extension, extensions.mesh) == 0) {
      outMeshFile << "BakedMesh(" << fileName << ",\"" << filePath << "\")\n";
    } else if(strcmp(extension, extensions.prefab) == 0) {
      outPrefabFile << "BakedPrefab(" << fileName << ",\"" << filePath << "\")\n";
    }
  }

  outTexturesFile.close();
  outMeshFile.close();
  outMaterialFile.close();
  outPrefabFile.close();
}

bool convertImage(const fs::path& inputPath, ConverterState& converterState)
{
	int texWidth, texHeight, texChannels;

	auto imageLoadStart = std::chrono::high_resolution_clock::now();

	stbi_uc* pixels = stbi_load(inputPath.u8string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	auto imageLoadEnd = std::chrono::high_resolution_clock::now();

	auto diff = imageLoadEnd - imageLoadStart;

	std::cout << "texture took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms to load"  << std::endl;

	if (!pixels) {
		std::cout << "Failed to load texture file " << inputPath << std::endl;
		return false;
	}

	TextureInfo texInfo;
  texInfo.textureSize = texWidth * texHeight * 4;
  texInfo.textureFormat = TextureFormat::RGBA8;
  texInfo.originalFile = inputPath.string();
  texInfo.width = texWidth;
  texInfo.height = texHeight;

  auto  compressionStart = std::chrono::high_resolution_clock::now();

  // TODO: Mipmaps
//  std::vector<char> textureBuffer_AllMipmaps;
//
//	struct DumbHandler : nvtt::OutputHandler {
//		// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
//		virtual bool writeData(const void* data, int size) {
//			for (int i = 0; i < size; i++) {
//				buffer.push_back(((char*)data)[i]);
//			}
//			return true;
//		}
//		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) { };
//
//		// Indicate the end of the compressed image. (New in NVTT 2.1)
//		virtual void endImage() {};
//		std::vector<char> buffer;
//	};
//
//	nvtt::Compressor compressor;
//
//	nvtt::CompressionOptions options;
//	nvtt::OutputOptions outputOptions;
//	nvtt::Surface surface;
//
//	DumbHandler handler;
//	outputOptions.setOutputHandler(&handler);
//
//	surface.setImage(nvtt::InputFormat::InputFormat_BGRA_8UB, texWidth, texHeight, 1, pixels);
//
//	while (surface.canMakeNextMipmap(1))
//	{
//    surface.buildNextMipmap(nvtt::MipmapFilter_Box);
//
//    options.setFormat(nvtt::Format::Format_RGBA);
//    options.setPixelType(nvtt::PixelType_UnsignedNorm);
//
//    compressor.compress(surface, 0, 0, options, outputOptions);
//
//    texInfo.pages.push_back({});
//    texInfo.pages.back().width = surface.width();
//    texInfo.pages.back().height = surface.height();
//    texInfo.pages.back().originalSize = handler.buffer.size();
//
//    textureBuffer_AllMipmaps.insert(textureBuffer_AllMipmaps.end(), handler.buffer.begin(), handler.buffer.end());
//    handler.buffer.clear();
//	}
//
// texInfo.textureSize = textureBuffer_AllMipmaps.size();

	assets::AssetFile newImage = assets::packTexture(&texInfo, pixels);

	auto  compressionEnd = std::chrono::high_resolution_clock::now();

  diff = compressionEnd - compressionStart;

	std::cout << "compression took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms" << std::endl;

	stbi_image_free(pixels);

  fs::path relative = inputPath.lexically_proximate(converterState.assetsDir);
  fs::path exportPath = converterState.bakedAssetDir / relative;
  exportPath.replace_extension(extensions.texture);

  saveAssetFile(exportPath.string().c_str(), newImage);
  converterState.bakedFilePaths.push_back(exportPath);

	return true;
}

void packVertex(assets::Vertex_PNCV_f32& new_vert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t ux, tinyobj::real_t uy)
{
	new_vert.position[0] = vx;
	new_vert.position[1] = vy;
	new_vert.position[2] = vz;

	new_vert.normal[0] = nx;
	new_vert.normal[1] = ny;
	new_vert.normal[2] = nz;

	new_vert.uv[0] = ux;
	new_vert.uv[1] = 1 - uy;
}

void packVertex(assets::Vertex_P32N8C8V16& new_vert, tinyobj::real_t vx, tinyobj::real_t vy, tinyobj::real_t vz, tinyobj::real_t nx, tinyobj::real_t ny, tinyobj::real_t nz, tinyobj::real_t ux, tinyobj::real_t uy)
{
	new_vert.position[0] = vx;
	new_vert.position[1] = vy;
	new_vert.position[2] = vz;

	new_vert.normal[0] = u8(  ((nx + 1.0) / 2.0) * 255);
	new_vert.normal[1] = u8(  ((ny + 1.0) / 2.0) * 255);
	new_vert.normal[2] = u8(  ((nz + 1.0) / 2.0) * 255);

	new_vert.uv[0] = ux;
	new_vert.uv[1] = 1 - uy;
}

void unpackGltfBuffer(tinygltf::Model& model, tinygltf::Accessor& accessor, std::vector<u8> &outputBuffer)
{
	int bufferID = accessor.bufferView;
	size_t elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);

	tinygltf::BufferView& bufferView = model.bufferViews[bufferID];	

	tinygltf::Buffer& bufferData = (model.buffers[bufferView.buffer]);


	u8* dataptr = bufferData.data.data() + accessor.byteOffset + bufferView.byteOffset;

	int components = tinygltf::GetNumComponentsInType(accessor.type);

	elementSize *= components;

	size_t stride = bufferView.byteStride;
	if (stride == 0)
	{
		stride = elementSize;
		
	}

	outputBuffer.resize(accessor.count * elementSize);

	for (int i = 0; i < accessor.count; i++) {
		u8* dataindex = dataptr + stride * i;
		u8* targetptr = outputBuffer.data() + elementSize * i;

		memcpy(targetptr, dataindex, elementSize);	
	}
}

void extractGltfVertices(tinygltf::Primitive& primitive, tinygltf::Model& model, std::vector<Vertex_PNCV_f32>& _vertices)
{
	
	tinygltf::Accessor& pos_accesor = model.accessors[primitive.attributes["POSITION"]];

	_vertices.resize(pos_accesor.count);

	std::vector<u8> pos_data;
  unpackGltfBuffer(model, pos_accesor, pos_data);
	

	for (int i = 0; i < _vertices.size(); i++) {
		if (pos_accesor.type == TINYGLTF_TYPE_VEC3)
		{
			if (pos_accesor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				float* dtf = (float*)pos_data.data();

				//vec3f 
				_vertices[i].position[0] = *(dtf + (i * 3) + 0);
				_vertices[i].position[1] = *(dtf + (i * 3) + 1);
				_vertices[i].position[2] = *(dtf + (i * 3) + 2);
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}

	tinygltf::Accessor& normal_accesor = model.accessors[primitive.attributes["NORMAL"]];

	std::vector<u8> normal_data;
  unpackGltfBuffer(model, normal_accesor, normal_data);


	for (int i = 0; i < _vertices.size(); i++) {
		if (normal_accesor.type == TINYGLTF_TYPE_VEC3)
		{
			if (normal_accesor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				float* dtf = (float*)normal_data.data();

				//vec3f 
				_vertices[i].normal[0] = *(dtf + (i * 3) + 0);
				_vertices[i].normal[1] = *(dtf + (i * 3) + 1);
				_vertices[i].normal[2] = *(dtf + (i * 3) + 2);

				_vertices[i].color[0] = *(dtf + (i * 3) + 0);
				_vertices[i].color[1] = *(dtf + (i * 3) + 1);
				_vertices[i].color[2] = *(dtf + (i * 3) + 2);
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}

	tinygltf::Accessor& uv_accesor = model.accessors[primitive.attributes["TEXCOORD_0"]];

	std::vector<u8> uv_data;
  unpackGltfBuffer(model, uv_accesor, uv_data);


	for (int i = 0; i < _vertices.size(); i++) {
		if (uv_accesor.type == TINYGLTF_TYPE_VEC2)
		{
			if (uv_accesor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				float* dtf = (float*)uv_data.data();

				//vec3f 
				_vertices[i].uv[0] = *(dtf + (i * 2) + 0);
				_vertices[i].uv[1] = *(dtf + (i * 2) + 1);
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
	}

	//for (auto& v : _vertices)
	//{
	//	v.position[0] *= -1;
	//
	//	v.normal[0] *= -1;
	//	v.normal[1] *= -1;
	//	v.normal[2] *= -1;
	//	//v.position = flip * glm::vec4(v.position, 1.f);
	//}

	return;
}

std::string calculateGltfMaterialName(tinygltf::Model& model, int materialIndex) {
  char buffer[50];

  itoa(materialIndex, buffer, 10);
  std::string matname = "MAT_" + std::string{ &buffer[0] } + "_" + model.materials[materialIndex].name;
  return matname;
}

void extractGltfIndices(tinygltf::Primitive& primitive, tinygltf::Model& model, std::vector<u32>& _primIndices)
{
	int indexaccesor = primitive.indices;	

	int indexbuffer = model.accessors[indexaccesor].bufferView;
	int componentType = model.accessors[indexaccesor].componentType;
	size_t indexsize = tinygltf::GetComponentSizeInBytes(componentType);

	tinygltf::BufferView& indexview = model.bufferViews[indexbuffer];
	int bufferidx = indexview.buffer;

	tinygltf::Buffer& buffindex = (model.buffers[bufferidx]);

	u8* dataptr = buffindex.data.data() + indexview.byteOffset;

	std::vector<u8> unpackedIndices;
  unpackGltfBuffer(model, model.accessors[indexaccesor], unpackedIndices);

	for (int i = 0; i < model.accessors[indexaccesor].count; i++) {

		u32 index;
		switch (componentType) {
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		{
			u16* bfr = (u16*)unpackedIndices.data();
			index = *(bfr + i);
		}
		break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		{
			int16_t* bfr = (int16_t*)unpackedIndices.data();
			index = *(bfr + i);
		}
		break;
		default:
			assert(false);
		}

		_primIndices.push_back(index);
	}

	for (int i = 0; i < _primIndices.size() / 3; i++)
	{
		//flip the triangle

		std::swap(_primIndices[i * 3 + 1], _primIndices[i * 3 + 2]);
	}
}

std::string calculateGltfMeshName(tinygltf::Model& model, int meshIndex, int primitiveIndex)
{
	char buffer0[50];
	char buffer1[50];
	itoa(meshIndex, buffer0, 10);
	itoa(primitiveIndex, buffer1, 10);

	std::string meshname = "MESH_" + std::string{ &buffer0[0] } + "_" + model.meshes[meshIndex].name;

	bool multiprim = model.meshes[meshIndex].primitives.size() > 1;
	if (multiprim)
	{
		meshname += "_PRIM_" + std::string{ &buffer1[0] };

		
	}
	
	return meshname;
}

bool extractGltfMeshes(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState)
{
	tinygltf::Model* gltfMod = &model;
	for (auto meshindex = 0; meshindex < model.meshes.size(); meshindex++){

		tinygltf::Mesh& gltfMesh = model.meshes[meshindex];
		

		using VertexFormat = assets::Vertex_PNCV_f32;
		auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;

		std::vector<VertexFormat> vertices;
		std::vector<u32> indices;

		for (auto primindex = 0; primindex < gltfMesh.primitives.size(); primindex++){

			vertices.clear();
			indices.clear();

			std::string meshName = calculateGltfMeshName(model, meshindex, primindex);

			tinygltf::Primitive& primitive = gltfMesh.primitives[primindex];

      extractGltfIndices(primitive, model, indices);
      extractGltfVertices(primitive, model, vertices);
			

			MeshInfo meshInfo;
      meshInfo.vertexFormat = VertexFormatEnum;
      meshInfo.vertexBufferSize = vertices.size() * sizeof(VertexFormat);
      meshInfo.indexBufferSize = indices.size() * sizeof(u32);
      meshInfo.indexSize = sizeof(u32);
      meshInfo.originalFile = input.string();

      meshInfo.bounds = assets::calculateBounds(vertices.data(), vertices.size());

			assets::AssetFile newFile = assets::packMesh(&meshInfo, (char*)vertices.data(), (char*)indices.data());

			fs::path meshPath = outputFolder/(meshName + extensions.mesh);

			//save to disk
      saveAssetFile(meshPath.string().c_str(), newFile);
      converterState.bakedFilePaths.push_back(meshPath);
		}
	}
	return true;
}

void extractGltfMaterials(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState)
{

	int materialIndex = 0;
	for (tinygltf::Material& gltfMat : model.materials) {
		std::string matName = calculateGltfMaterialName(model, materialIndex++);

		tinygltf::PbrMetallicRoughness& pbr = gltfMat.pbrMetallicRoughness;

		assets::MaterialInfo newMaterial;
		newMaterial.baseEffect = "defaultPBR";

		if (pbr.baseColorTexture.index >= 0) {
			tinygltf::Texture baseColor = model.textures[pbr.baseColorTexture.index];
			tinygltf::Image baseImage = model.images[baseColor.source];

			fs::path baseColorPath = outputFolder.parent_path()/baseImage.uri;

			baseColorPath.replace_extension(extensions.texture);

			baseColorPath = converterState.convertToExportRelative(baseColorPath);

			newMaterial.textures["baseColor"] = baseColorPath.string();
		}
		if (pbr.metallicRoughnessTexture.index >= 0)
		{
      tinygltf::Texture image = model.textures[pbr.metallicRoughnessTexture.index];
      tinygltf::Image baseImage = model.images[image.source];

			fs::path baseColorPath = outputFolder.parent_path()/baseImage.uri;

			baseColorPath.replace_extension(extensions.texture);

			baseColorPath = converterState.convertToExportRelative(baseColorPath);

			newMaterial.textures["metallicRoughness"] = baseColorPath.string();
		}

		if (gltfMat.normalTexture.index >= 0)
		{
      tinygltf::Texture image = model.textures[gltfMat.normalTexture.index];
      tinygltf::Image baseImage = model.images[image.source];

			fs::path baseColorPath = outputFolder.parent_path()/baseImage.uri;

			baseColorPath.replace_extension(extensions.texture);

			baseColorPath = converterState.convertToExportRelative(baseColorPath);

			newMaterial.textures["normals"] = baseColorPath.string();
		}

		if (gltfMat.occlusionTexture.index >= 0)
		{
      tinygltf::Texture image = model.textures[gltfMat.occlusionTexture.index];
      tinygltf::Image baseImage = model.images[image.source];

			fs::path baseColorPath = outputFolder.parent_path()/baseImage.uri;

			baseColorPath.replace_extension(extensions.texture);

			baseColorPath = converterState.convertToExportRelative(baseColorPath);

			newMaterial.textures["occlusion"] = baseColorPath.string();
		}

		if (gltfMat.emissiveTexture.index >= 0)
		{
      tinygltf::Texture image = model.textures[gltfMat.emissiveTexture.index];
      tinygltf::Image baseImage = model.images[image.source];

			fs::path baseColorPath = outputFolder.parent_path()/baseImage.uri;

			baseColorPath.replace_extension(extensions.texture);

			baseColorPath = converterState.convertToExportRelative(baseColorPath);

			newMaterial.textures["emissive"] = baseColorPath.string();
		}

		fs::path materialPath = outputFolder/(matName + extensions.material);

		if (gltfMat.alphaMode.compare("BLEND") == 0)
		{
			newMaterial.transparency = TransparencyMode::Transparent;
		}
		else {
			newMaterial.transparency = TransparencyMode::Opaque;
		}

		assets::AssetFile newFile = assets::packMaterial(&newMaterial);

		//save to disk
    saveAssetFile(materialPath.string().c_str(), newFile);
    converterState.bakedFilePaths.push_back(materialPath);
	}
}

void extractGltfNodes(tinygltf::Model& model, const fs::path& input, const fs::path& outputFolder, ConverterState& converterState) {
  assets::PrefabInfo prefab;

  std::vector<u64> meshNodes;
  for (int i = 0; i < model.nodes.size(); i++)
  {
    auto& node = model.nodes[i];

    std::string nodeName = node.name;

    prefab.nodeNames[i] = nodeName;

    std::array<float, 16> matrix;

    //node has a matrix
    if (node.matrix.size() > 0)
    {
      for (int n = 0; n < 16; n++) {
        matrix[n] = (f32)node.matrix[n];
      }

      //glm::mat4 flip = glm::mat4{ 1.0 };
      //flip[1][1] = -1;

      glm::mat4 mat;

      memcpy(&mat, &matrix, sizeof(glm::mat4));

      mat = mat;// * flip;

      memcpy(matrix.data(), &mat, sizeof(glm::mat4));
    }
      //separate transform
    else
    {
      glm::mat4 translation{ 1.f };
      if (node.translation.size() > 0)
      {
        translation = glm::translate(glm::vec3{ (f32)node.translation[0],
                                                (f32)node.translation[1] ,
                                                (f32)node.translation[2] });
      }

      glm::mat4 rotation{ 1.f };
      if (node.rotation.size() > 0)
      {
        glm::quat rot( (f32)node.rotation[3],
                       (f32)node.rotation[0],
                       (f32)node.rotation[1],
                       (f32)node.rotation[2]);
        rotation = glm::mat4{rot};
      }

      glm::mat4 scale{ 1.f };
      if (node.scale.size() > 0)
      {
        scale = glm::scale(glm::vec3{ (f32)node.scale[0],
                                      (f32)node.scale[1] ,
                                      (f32)node.scale[2] });
      }
      //glm::mat4 flip = glm::mat4{ 1.0 };
      //flip[1][1] = -1;

      glm::mat4 transformMatrix = (translation * rotation * scale);// * flip;

      memcpy(matrix.data(), &transformMatrix, sizeof(glm::mat4));
    }

    prefab.nodeMatrices[i] = (u32)prefab.matrices.size();
    prefab.matrices.push_back(matrix);

    if (node.mesh >= 0)
    {
      auto mesh = model.meshes[node.mesh];

      if (mesh.primitives.size() > 1) {
        meshNodes.push_back(i);
      }
      else {
        auto primitive = mesh.primitives[0];
        std::string meshName = calculateGltfMeshName(model, node.mesh, 0);

        fs::path meshPath = outputFolder/(meshName + extensions.mesh);

        int material = primitive.material;

        std::string matName = calculateGltfMaterialName(model, material);

        fs::path materialPath = outputFolder/(matName + extensions.material);

        assets::PrefabInfo::NodeMesh nodeMesh;
        nodeMesh.meshPath = converterState.convertToExportRelative(meshPath).string();
        nodeMesh.materialPath = converterState.convertToExportRelative(materialPath).string();

        prefab.nodeMeshes[i] = nodeMesh;
      }
    }
  }

  //calculate parent hierarchies
  //gltf stores children, but we want parent
  for (int i = 0; i < model.nodes.size(); i++)
  {
    for (auto c : model.nodes[i].children)
    {
      prefab.nodeParents[c] = i;
    }
  }

  //for every gltf node that is a root node (no parents), apply the coordinate fixup

  glm::mat4 flip = glm::mat4{ 1.0 };
  flip[1][1] = -1;


  glm::mat4 rotation = glm::mat4{ 1.0 };
  //flip[1][1] = -1;
  rotation = glm::rotate(glm::radians(-180.f), glm::vec3{ 1,0,0 });


  //flip[2][2] = -1;
  for (int i = 0; i < model.nodes.size(); i++)
  {
    auto it = prefab.nodeParents.find(i);
    if (it == prefab.nodeParents.end())
    {
      auto matrix = prefab.matrices[prefab.nodeMatrices[i]];
      //no parent, root node
      glm::mat4 mat;

      memcpy(&mat, &matrix, sizeof(glm::mat4));

      mat =rotation*(flip* mat);

      memcpy(&matrix, &mat, sizeof(glm::mat4));

      prefab.matrices[prefab.nodeMatrices[i]] = matrix;
    }
  }

  size_t nodeIndex = model.nodes.size();
  //iterate nodes with mesh, convert each submesh into a node
  for (int i = 0; i < meshNodes.size(); i++)
  {
    tinygltf::Node& node = model.nodes[i];

    if (node.mesh < 0) break;

    tinygltf::Mesh mesh = model.meshes[node.mesh];

    for (int primindex = 0 ; primindex < mesh.primitives.size(); primindex++)
    {
      tinygltf::Primitive primitive = mesh.primitives[primindex];

      char buffer[50];

      itoa(primindex, buffer, 10);

      prefab.nodeNames[nodeIndex] = prefab.nodeNames[i] + "_PRIM_" + &buffer[0];

      int material = primitive.material;
      auto mat = model.materials[material];
      std::string matName = calculateGltfMaterialName(model, material);
      std::string meshName = calculateGltfMeshName(model, node.mesh, primindex);

      fs::path materialPath = outputFolder/(matName + extensions.material);
      fs::path meshPath = outputFolder/(meshName + extensions.mesh);

      assets::PrefabInfo::NodeMesh nodeMesh;
      nodeMesh.meshPath = converterState.convertToExportRelative(meshPath).string();
      nodeMesh.materialPath = converterState.convertToExportRelative(materialPath).string();

      prefab.nodeMeshes[nodeIndex] = nodeMesh;
      nodeIndex++;
    }
  }


  assets::AssetFile newFile = assets::packPrefab(prefab);

  fs::path sceneFilePath = (outputFolder.parent_path())/input.stem();

  sceneFilePath.replace_extension(extensions.prefab);

  //save to disk
  saveAssetFile(sceneFilePath.string().c_str(), newFile);
  converterState.bakedFilePaths.push_back(sceneFilePath);
}

fs::path ConverterState::convertToExportRelative(const fs::path& path) const
{
	return path.lexically_proximate(bakedAssetDir);
}

std::size_t fileCountInDir(fs::path dirPath)
{
  std::size_t fileCount = 0u;
  for (auto const & file : std::filesystem::directory_iterator(dirPath))
  {
    if(fs::is_regular_file(file)) {
      ++fileCount;
    }
  }
  return fileCount;
}

//bool convertObjMesh(const fs::path& input, const fs::path& output)
//{
//  //attrib will contain the assets::Vertex_f32_PNCV arrays of the file
//  tinyobj::attrib_t attrib;
//  //shapes contains the info for each separate object in the file
//  std::vector<tinyobj::shape_t> shapes;
//  //materials contains the information about the material of each shape, but we wont use it.
//  std::vector<tinyobj::material_t> materials;
//
//  //error and warning output from the load function
//  std::string warn;
//  std::string err;
//  auto pngstart = std::chrono::high_resolution_clock::now();
//
//  //load the OBJ file
//  tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, input.string().c_str(),
//                   nullptr);
//
//  auto pngend = std::chrono::high_resolution_clock::now();
//
//  auto diff = pngend - pngstart;
//
//  std::cout << "obj took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms" << std::endl;
//
//  //make sure to output the warnings to the console, in case there are issues with the file
//  if (!warn.empty()) {
//    std::cout << "WARN: " << warn << std::endl;
//  }
//  //if we have any error, print it to the console, and break the mesh loading.
//  //This happens if the file cant be found or is malformed
//  if (!err.empty()) {
//    std::cerr << err << std::endl;
//    return false;
//  }
//
//  using VertexFormat = assets::Vertex_PNCV_f32;
//  auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;
//
//  std::vector<VertexFormat> _vertices;
//  std::vector<u32> _indices;
//
//  extractMeshFromObj(shapes, attrib, _indices, _vertices);
//
//  MeshInfo meshinfo;
//  meshinfo.vertexFormat = VertexFormatEnum;
//  meshinfo.vertexBufferSize = _vertices.size() * sizeof(VertexFormat);
//  meshinfo.indexBufferSize = _indices.size() * sizeof(u32);
//  meshinfo.indexSize = sizeof(u32);
//  meshinfo.originalFile = input.string();
//
//  meshinfo.bounds = assets::calculateBounds(_vertices.data(), _vertices.size());
//  //pack mesh file
//  auto start = std::chrono::high_resolution_clock::now();
//
//  assets::AssetFile newFile = assets::packMesh(&meshinfo, (char*)_vertices.data(), (char*)_indices.data());
//
//  auto  end = std::chrono::high_resolution_clock::now();
//
//  diff = end - start;
//
//  std::cout << "compression took " << std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count() / 1000000.0 << "ms" << std::endl;
//
//  //save to disk
//  saveAssetFile(output.string().c_str(), newFile);
//
//  return true;
//}

//template<typename V>
//void extractMeshFromObj(std::vector<tinyobj::shape_t>& shapes, tinyobj::attrib_t& attrib, std::vector<u32>& _indices, std::vector<V>& _vertices)
//{
//  // Loop over shapes
//  for (size_t s = 0; s < shapes.size(); s++) {
//    // Loop over faces(polygon)
//    size_t index_offset = 0;
//    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
//
//      //hardcode loading to triangles
//      int fv = 3;
//
//      // Loop over vertices in the face.
//      for (size_t v = 0; v < fv; v++) {
//        // access to assets::Vertex_f32_PNCV
//        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
//
//        //vertex position
//        tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
//        tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
//        tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
//        //vertex normal
//        tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
//        tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
//        tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
//
//        //vertex uv
//        tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
//        tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];
//
//        //copy it into our vertex
//        V new_vert;
//        packVertex(new_vert, vx, vy, vz, nx, ny, nz, ux, uy);
//
//
//        _indices.push_back(_vertices.size());
//        _vertices.push_back(new_vert);
//      }
//      index_offset += fv;
//    }
//  }
//}
//
//std::string calculateAssimpMeshName(const aiScene* scene, int meshIndex)
//{
//  char buffer[50];
//
//  itoa(meshIndex, buffer, 10);
//  std::string matname = "MESH_" + std::string{ buffer } + "_"+ std::string{ scene->mMeshes[meshIndex]->mName.C_Str()};
//  return matname;
//}
//
//std::string calculateAssimpMaterialName(const aiScene* scene, int materialIndex)
//{
//  char buffer[50];
//
//  itoa(materialIndex, buffer, 10);
//  std::string matname = "MAT_" + std::string{ buffer } + "_" + std::string{ scene->mMaterials[materialIndex]->GetName().C_Str() };
//  return matname;
//}
//
//void extractAssimpMaterials(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState)
//{
//  for (int m = 0; m < scene->mNumMaterials; m++) {
//    std::string matname = calculateAssimpMaterialName(scene, m);
//
//    assets::MaterialInfo newMaterial;
//    newMaterial.baseEffect = "defaultPBR";
//
//    aiMaterial* material = scene->mMaterials[m];
//    newMaterial.transparencyMode = TransparencyMode::Transparent;
//    for (int p = 0; p < material->mNumProperties; p++)
//    {
//      aiMaterialProperty* pt = material->mProperties[p];
//      switch (pt->mType )
//      {
//        case aiPTI_String:
//        {
//          const char* data = pt->mData;
//          newMaterial.customProperties[pt->mKey.C_Str()] = data;
//        }
//          break;
//        case aiPTI_Float:
//        {
//          std::stringstream ss;
//          ss << *(float*)pt->mData;
//          newMaterial.customProperties[pt->mKey.C_Str()] = ss.str();
//
//          if (strcmp(pt->mKey.C_Str(), "$mat.opacity") == 0)
//          {
//            float num = *(float*)pt->mData;
//            if (num != 1.0)
//            {
//              newMaterial.transparencyMode = TransparencyMode::Transparent;
//            }
//          }
//        }
//          break;
//      }
//    }
//
//    //check opacity
//
//
//    std::string texPath = "";
//    if (material->GetTextureCount(aiTextureType_DIFFUSE))
//    {
//      aiString assimppath;
//      material->GetTexture(aiTextureType_DIFFUSE, 0, &assimppath);
//
//      fs::path texturePath = &assimppath.data[0];
//      //unreal compat
//      texturePath =  texturePath.filename();
//      texPath = "T_" + texturePath.string();
//    }
//    else if (material->GetTextureCount(aiTextureType_BASE_COLOR))
//    {
//      aiString assimppath;
//      material->GetTexture(aiTextureType_BASE_COLOR, 0, &assimppath);
//
//      fs::path texturePath = &assimppath.data[0];
//      //unreal compat
//      texturePath = texturePath.filename();
//      texPath = "T_" + texturePath.string();
//    }
//      //force a default texture
//    else {
//      texPath = "Default";
//    }
//    fs::path baseColorPath = outputFolder.parent_path()/texPath;
//
//    baseColorPath.replace_extension(extensions.texture);
//
//    baseColorPath = convState.convertToExportRelative(baseColorPath);
//
//    newMaterial.textures["baseColor"] = baseColorPath.string();
//
//    fs::path materialPath = outputFolder/(matname + extensions.material);
//
//
//
//    assets::AssetFile newFile = assets::packMaterial(&newMaterial);
//
//    //save to disk
//    saveAssetFile(materialPath.string().c_str(), newFile);
//  }
//}
//
//void extractAssimpMeshes(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState)
//{
//  for (int meshindex = 0; meshindex < scene->mNumMeshes; meshindex++) {
//
//    auto mesh = scene->mMeshes[meshindex];
//
//    using VertexFormat = assets::Vertex_PNCV_f32;
//    auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;
//
//    std::vector<VertexFormat> _vertices;
//    std::vector<u32> _indices;
//
//    std::string meshName = calculateAssimpMeshName(scene, meshindex);
//
//    _vertices.resize(mesh->mNumVertices);
//    for (int v = 0; v < mesh->mNumVertices; v++)
//    {
//      VertexFormat vert;
//      vert.position[0] = mesh->mVertices[v].x;
//      vert.position[1] = mesh->mVertices[v].y;
//      vert.position[2] = mesh->mVertices[v].z;
//
//      vert.normal[0] = mesh->mNormals[v].x;
//      vert.normal[1] = mesh->mNormals[v].y;
//      vert.normal[2] = mesh->mNormals[v].z;
//
//      if (mesh->GetNumUVChannels() >= 1)
//      {
//        vert.uv[0] = mesh->mTextureCoords[0][v].x;
//        vert.uv[1] = mesh->mTextureCoords[0][v].y;
//      }
//      else {
//        vert.uv[0] =0;
//        vert.uv[1] = 0;
//      }
//      if (mesh->HasVertexColors(0))
//      {
//        vert.color[0] = mesh->mColors[0][v].r;
//        vert.color[1] = mesh->mColors[0][v].g;
//        vert.color[2] = mesh->mColors[0][v].b;
//      }
//      else {
//        vert.color[0] =1;
//        vert.color[1] =1;
//        vert.color[2] =1;
//      }
//
//      _vertices[v] = vert;
//    }
//    _indices.resize(mesh->mNumFaces * 3);
//    for (int f= 0; f < mesh->mNumFaces; f++)
//    {
//      _indices[f * 3 + 0] = mesh->mFaces[f].mIndices[0];
//      _indices[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
//      _indices[f * 3 + 2] = mesh->mFaces[f].mIndices[2];
//
//      //assimp fbx creates bad normals, just regen them
//      if (true)
//      {
//        int v0 = _indices[f * 3 + 0];
//        int v1 = _indices[f * 3 + 1];
//        int v2 = _indices[f * 3 + 2];
//        glm::vec3 p0{ _vertices[v0].position[0],
//                      _vertices[v0].position[1],
//                      _vertices[v0].position[2]
//        };
//        glm::vec3 p1{ _vertices[v1].position[0],
//                      _vertices[v1].position[1],
//                      _vertices[v1].position[2]
//        };
//        glm::vec3 p2{ _vertices[v2].position[0],
//                      _vertices[v2].position[1],
//                      _vertices[v2].position[2]
//        };
//
//        glm::vec3 normal =  glm::normalize(glm::cross(p2 - p0, p1 - p0));
//
//        memcpy(_vertices[v0].normal, &normal, sizeof(float) * 3);
//        memcpy(_vertices[v1].normal, &normal, sizeof(float) * 3);
//        memcpy(_vertices[v2].normal, &normal, sizeof(float) * 3);
//      }
//    }
//
//    MeshInfo meshInfo;
//    meshInfo.vertexFormat = VertexFormatEnum;
//    meshInfo.vertexBufferSize = _vertices.size() * sizeof(VertexFormat);
//    meshInfo.indexBufferSize = _indices.size() * sizeof(u32);
//    meshInfo.indexSize = sizeof(u32);
//    meshInfo.originalFile = input.string();
//
//    meshInfo.bounds = assets::calculateBounds(_vertices.data(), _vertices.size());
//
//    assets::AssetFile newFile = assets::packMesh(&meshInfo, (char*)_vertices.data(), (char*)_indices.data());
//
//    fs::path meshPath = outputFolder/(meshName + extensions.mesh);
//
//    //save to disk
//    saveAssetFile(meshPath.string().c_str(), newFile);
//  }
//}
//
//void extractAssimpNodes(const aiScene* scene, const fs::path& input, const fs::path& outputFolder, ConverterState& convState)
//{
//
//  assets::PrefabInfo prefab;
//
//  glm::mat4 ident{1.f};
//
//  std::array<float, 16> identityMatrix;
//  memcpy(&identityMatrix, &ident, sizeof(glm::mat4));
//
//
//  u64 lastNode = 0;
//  std::function<void(aiNode* node, aiMatrix4x4& parentmat, u64)> process_node = [&](aiNode* node, aiMatrix4x4& parentmat, u64 parentID) {
//
//    aiMatrix4x4 node_mat = /*parentmat * */node->mTransformation;
//
//    glm::mat4 modelmat;
//    for (int y = 0; y < 4; y++)
//    {
//      for (int x = 0; x < 4; x++)
//      {
//        modelmat[y][x] = node_mat[x][y];
//      }
//    }
//
//    u64 nodeindex = lastNode;
//    lastNode++;
//
//    std::array<float, 16> matrix;
//    memcpy(&matrix, &modelmat, sizeof(glm::mat4));
//
//    if (parentID != nodeindex)
//    {
//      prefab.nodeParents[nodeindex] = parentID;
//    }
//
//    prefab.nodeMatrices[nodeindex] = prefab.matrices.size();
//    prefab.matrices.push_back(matrix);
//
//
//    std::string nodename = node->mName.C_Str();
//    //std::cout << nodename << std::endl;
//
//    if (nodename.size() > 0)
//    {
//      prefab.nodeNames[nodeindex] = nodename;
//    }
//    for (int msh = 0; msh < node->mNumMeshes; msh++) {
//
//      int mesh_index = node->mMeshes[msh];
//      std::string meshName = "Mesh: " + std::string{scene->mMeshes[mesh_index]->mName.C_Str() };
//
//      //std::cout << meshName << std::endl;
//
//      std::string materialName = calculateAssimpMaterialName(scene, scene->mMeshes[mesh_index]->mMaterialIndex);
//      meshName = calculateAssimpMeshName(scene, mesh_index);
//
//      fs::path materialPath = outputFolder/(materialName + extensions.material);
//      fs::path meshPath = outputFolder/(meshName + extensions.mesh);
//
//      assets::PrefabInfo::NodeMesh nmesh;
//      nmesh.meshPath = convState.convertToExportRelative(meshPath).string();
//      nmesh.materialPath = convState.convertToExportRelative(materialPath).string();
//      u64 newNode = lastNode; lastNode++;
//
//      prefab.nodeMeshes[newNode] = nmesh;
//      prefab.nodeParents[newNode] = nodeindex;
//
//      prefab.nodeMatrices[newNode] = prefab.matrices.size();
//      prefab.matrices.push_back(identityMatrix);
//    }
//
//    for (int ch = 0; ch < node->mNumChildren; ch++)
//    {
//      process_node(node->mChildren[ch], node_mat,nodeindex);
//    }
//  };
//
//  aiMatrix4x4 mat{};
//  glm::mat4 rootMat{1};// (, rootMatrix.v);
//
//  for (int y = 0; y < 4; y++)
//  {
//    for (int x = 0; x < 4; x++)
//    {
//      mat[x][y] = rootMat[y][x];
//    }
//  }
//
//  process_node(scene->mRootNode, mat,0);
//
//  assets::AssetFile newFile = assets::packPrefab(prefab);
//
//  fs::path sceneFilePath = (outputFolder.parent_path())/input.stem();
//
//  sceneFilePath.replace_extension(extensions.prefab);
//
//  //save to disk
//  saveAssetFile(sceneFilePath.string().c_str(), newFile);
//}