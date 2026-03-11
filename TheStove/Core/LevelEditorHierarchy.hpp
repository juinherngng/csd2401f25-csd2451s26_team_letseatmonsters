/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorHierarchy.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Header for the Level Editor hierarchy panel, which displays a list of scene objects with labels and supports filtering by name.
					Caches generated labels for performance and prunes cache entries for deleted objects.
					Provides utilities for checking if an object passes the current filter based on its cached label.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

class Scene;

namespace LEHIERARCHY {
	// Cache entry for storing generated labels and search tokens for scene objects in the hierarchy panel.
	void InvalidateCache();

	// Retrieves the cached label for a given object ID, generating and caching it if necessary based on the object's tag, texture path, and layer.
	const std::string& GetCachedLabel(Scene& scene, int objectId);

	// Prunes cache entries for objects that no longer exist in the scene by comparing the set of live object IDs with the keys in the cache and erasing any entries that do not correspond to live objects.
	void PruneDeadObjects(Scene& scene);

	// Checks if a given label passes the current filter by converting both the label and filter to lowercase and checking if the filter is a substring of the label.
	bool PassesFilter(const std::string& label, const std::string& filterLower);

	// Checks if a given label passes the current filter by converting both the label and filter to lowercase and checking if the filter is a substring of the label. 
	bool PassesFilterCached(int objectId, const std::string& filterLower);

	// Utility function to convert a string to lowercase. Used for generating search tokens for filtering.
	std::string ToLowerCopy(std::string value);
}
