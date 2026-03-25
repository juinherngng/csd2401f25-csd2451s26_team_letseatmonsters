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
	/**
	 * @brief Clears cached hierarchy labels and search tokens.
	 */
	void InvalidateCache();

	/**
	 * @brief Returns the cached hierarchy label for an object.
	 * @param scene Scene containing the object.
	 * @param objectId Engine object identifier.
	 * @return Cached display label for the requested object.
	 */
	const std::string& GetCachedLabel(Scene& scene, int objectId);

	/**
	 * @brief Removes cached hierarchy entries for objects that no longer exist.
	 * @param scene Scene used to determine which object IDs are still valid.
	 */
	void PruneDeadObjects(Scene& scene);

	/**
	 * @brief Tests whether a hierarchy label matches the active filter text.
	 * @param label Display label to evaluate.
	 * @param filterLower Lowercase filter string entered by the user.
	 * @return True when the label passes the filter.
	 */
	bool PassesFilter(const std::string& label, const std::string& filterLower);

	/**
	 * @brief Tests whether a cached object label matches the active filter text.
	 * @param objectId Engine object identifier.
	 * @param filterLower Lowercase filter string entered by the user.
	 * @return True when the cached label passes the filter.
	 */
	bool PassesFilterCached(int objectId, const std::string& filterLower);

	/**
	 * @brief Converts a string to lowercase for case-insensitive comparisons.
	 * @param value Input string to convert.
	 * @return Lowercase copy of the input string.
	 */
	std::string ToLowerCopy(std::string value);
}
