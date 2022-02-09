#pragma once

struct {
#define BakedMaterial(name, filePath) const char* name = filePath;
#include "../assets_metadata/baked_materials.incl"
#undef BakedMaterial
} bakedMaterials;

struct {
#define BakedMesh(name, filePath) const char* name = filePath;
#include "../assets_metadata/baked_meshes.incl"
#undef BakedMesh
} bakedMeshes;

struct {
#define BakedTexture(name, filePath) const char* name = filePath;
#include "../assets_metadata/baked_textures.incl"
#undef BakedTexture
} bakedTextures;

struct {
#define BakedPrefab(name, filePath) const char* name = filePath;
#include "../assets_metadata/baked_prefabs.incl"
#undef BakedPrefab
} bakedPrefabs;