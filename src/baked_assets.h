#pragma once

struct BakedAssetData {
  const char* name;
  const char* filePath;
};

struct BakedMaterials {
#define BakedMaterial(name, filePath) const BakedAssetData name = {#name, filePath};
#include "../assets_metadata/baked_materials.incl"
#undef BakedMaterial
} bakedMaterialAssetData;

struct BakedMeshes {
#define BakedMesh(name, filePath) const BakedAssetData name = {#name, filePath};
#include "../assets_metadata/baked_meshes.incl"
#undef BakedMesh
} bakedMeshAssetData;

struct BakedTextures {
#define BakedTexture(name, filePath) const BakedAssetData name = {#name, filePath};
#include "../assets_metadata/baked_textures.incl"
#undef BakedTexture
} bakedTextureAssetData;

struct BakedPrefabs {
#define BakedPrefab(name, filePath) const BakedAssetData name = {#name, filePath};
#include "../assets_metadata/baked_prefabs.incl"
#undef BakedPrefab
} bakedPrefabAssetData;

u32 bakedMeshAssetCount() {
  return (sizeof(BakedMeshes) / sizeof(BakedAssetData));
}

u32 bakedTextureAssetCount() {
  return (sizeof(BakedTextures) / sizeof(BakedAssetData));
}

u32 bakedMaterialAssetCount() {
  return (sizeof(BakedMaterials) / sizeof(BakedAssetData));
}

u32 bakedPrefabAssetCount() {
  return (sizeof(BakedPrefabs) / sizeof(BakedAssetData));
}