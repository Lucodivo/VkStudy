#pragma once

#include <asset_loader.h>

#include <unordered_map>

namespace assets {
	enum class TransparencyMode : u32 {
    Unknown = 0,
#define TransparencyMode(name) name,
#include "transparency_mode.incl"
#undef TransparencyMode
	};

	struct MaterialInfo {
		std::string baseEffect;
		std::unordered_map<std::string, std::string> textures; //name -> path
		std::unordered_map<std::string, std::string> customProperties;
		TransparencyMode transparency;
	};

	MaterialInfo readMaterialInfo(AssetFile* file);
	AssetFile packMaterial(MaterialInfo* info);
}