/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorHierarchy.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of the Level Editor hierarchy panel utilities, including caching of
					generated labels for scene objects to optimize filtering performance and pruning of
					cache entries for deleted objects. Provides functions to check if an object passes
					the current filter based on its cached label.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include "EngineCore/LevelEditorHierarchy.hpp"
#include "EngineGraphics/SceneManager.hpp"

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

	/**
	 * @brief Converts a string to lowercase for filtering and cache keys.
	 * @param value Input string to convert.
	 * @return Lowercase copy of the input string.
	 */
	std::string ToLowerCopy(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	/**
	 * @brief Tests whether a hierarchy label matches the current filter.
	 * @param label Display label to evaluate.
	 * @param filterLower Lowercase filter string entered by the user.
	 * @return True when the label passes the filter.
	 */
	bool PassesFilter(const std::string& label, const std::string& filterLower) {
		if (filterLower.empty()) {
			return true;
		}

		return ToLowerCopy(label).find(filterLower) != std::string::npos;
	}

	/**
	 * @brief Tests whether a cached hierarchy label matches the current filter.
	 * @param objectId Engine object identifier.
	 * @param filterLower Lowercase filter string entered by the user.
	 * @return True when the cached label passes the filter.
	 */
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

	/**
	 * @brief Returns a cached hierarchy label for an object, rebuilding it when needed.
	 * @param scene Scene containing the object.
	 * @param objectId Engine object identifier.
	 * @return Cached label string for the object.
	 */
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
			? ("id:" + std::to_string(objectId) + "  [L" + layer + "]")
			: (niceName + "  [id:" + std::to_string(objectId) + "] [L" + layer + "]");
		entry.searchTokenLower = ToLowerCopy(entry.label);

		return entry.label;
	}

	/**
	 * @brief Clears the hierarchy label cache.
	 */
	void InvalidateCache() {
		sHierarchyLabelCache.clear();
	}

	/**
	 * @brief Removes cache entries for objects that no longer exist in the scene.
	 * @param scene Scene used to determine which object IDs are still alive.
	 */
	void PruneDeadObjects(Scene& scene) {
		std::unordered_set<int> liveIds;
		auto objectList = scene.GetAllObjectsRaw();
		liveIds.reserve(objectList.size());
		for (auto* g : objectList) {
			if (g) {
				liveIds.insert(g->GetID());
			}
		}

		// Erase cache entries for IDs that are no longer live in the scene. Iterate with an iterator to safely erase while iterating.
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
