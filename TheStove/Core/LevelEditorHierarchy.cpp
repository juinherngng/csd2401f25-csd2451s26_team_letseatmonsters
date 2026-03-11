/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorHierarchy.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of the Level Editor hierarchy panel utilities, including caching of
					generated labels for scene objects to optimize filtering performance and pruning of
					cache entries for deleted objects. Provides functions to check if an object passes
					the current filter based on its cached label.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"

#include "LevelEditorHierarchy.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {
	// Cache entry for storing generated labels and search tokens for scene objects in the hierarchy panel.
	struct HierarchyLabelCacheEntry {
		std::string tag;
		std::string texturePath;
		std::string layer;
		std::string label;
		std::string searchTokenLower;
	};

	std::unordered_map<int, HierarchyLabelCacheEntry> sHierarchyLabelCache;
}

namespace LEHIERARCHY {
	std::string ToLowerCopy(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	// Checks if a given label passes the current filter by converting both the label and filter to lowercase and checking if the filter is a substring of the label.
	bool PassesFilter(const std::string& label, const std::string& filterLower) {
		if (filterLower.empty()) {
			return true;
		}

		return ToLowerCopy(label).find(filterLower) != std::string::npos;
	}

	bool PassesFilterCached(int objectId, const std::string& filterLower) {
		if (filterLower.empty()) {
			return true;
		}

		auto it = sHierarchyLabelCache.find(objectId);
		if (it == sHierarchyLabelCache.end()) {
			return false;
		}

		return it->second.searchTokenLower.find(filterLower) != std::string::npos;
	}

	// Retrieves the cached label for a given object ID, generating and caching it if necessary based on the object's tag, texture path, and layer.
	const std::string& GetCachedLabel(Scene& scene, int objectId) {
		auto& entry = sHierarchyLabelCache[objectId];
		const Scene::Defaults defs = scene.GetDefaults(objectId);
		const std::string texturePath = scene.GetObjectTexturePath(objectId);
		const std::string layer = scene.GetObjectLayer(objectId);

		if (entry.tag == defs.tag &&
			entry.texturePath == texturePath &&
			entry.layer == layer &&
			!entry.label.empty()) {
			return entry.label;
		}

		std::string niceName = defs.tag;
		if (niceName.empty() && !texturePath.empty()) {
			niceName = fs::path(texturePath).stem().string();
		}

		entry.tag = defs.tag;
		entry.texturePath = texturePath;
		entry.layer = layer;
		entry.label = niceName.empty()
			? ("ID " + std::to_string(objectId) + " [Layer: " + layer + "]")
			: (niceName + " (ID " + std::to_string(objectId) + ") [Layer: " + layer + "]");
		entry.searchTokenLower = ToLowerCopy(entry.label);

		return entry.label;
	}

	void InvalidateCache() {
		sHierarchyLabelCache.clear();
	}

	void PruneDeadObjects(Scene& scene) {
		std::unordered_set<int> liveIds;
		auto objectList = scene.GetAllObjectsRaw();
		liveIds.reserve(objectList.size());
		for (auto* g : objectList) {
			if (g) {
				liveIds.insert(g->GetID());
			}
		}

		for (auto it = sHierarchyLabelCache.begin(); it != sHierarchyLabelCache.end();) {
			if (liveIds.find(it->first) == liveIds.end()) {
				it = sHierarchyLabelCache.erase(it);
			}
			else {
				++it;
			}
		}
	}
}
