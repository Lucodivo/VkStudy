#pragma once

#include <asset_loader.h>

#include <unordered_map>

// Prefabs are just "prefabricated"
namespace assets {

	struct PrefabInfo {
		//points to matrix array in the blob
		std::unordered_map<u64, s32> nodeMatrices;
		std::unordered_map<u64, std::string> nodeNames;
		std::unordered_map<u64, u64> nodeParents;

		struct NodeMesh {
			std::string materialPath;
			std::string meshPath;
		};
		std::unordered_map<u64, NodeMesh> nodeMeshes;

		std::vector<std::array<f32, 16>> matrices;
	};

	PrefabInfo readPrefabInfo(AssetFile* file);
	AssetFile packPrefab(const PrefabInfo& info);
}