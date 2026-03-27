/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements RuntimeLevel utilities to parse LevelData JSON, spawn animated/static GameObjects
					with proper layers/tags/colliders/animations, set scene backgrounds, rebuild colliders, and
					store object metadata for runtime level loading. (For use outside of editor only.)

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>

#include "EngineCore/LevelSerializer.hpp"

class Scene;

namespace RuntimeLevel {
	struct LevelValidationReport {
		std::vector<std::string> warnings;

		/**
		 * @brief Returns whether the report contains warnings.
		 * @return True when the operation succeeds or the condition is met.
		 */
		bool HasWarnings() const {
			return !warnings.empty();
		}
	};

	// Build the current Scene from LevelData (runtime-friendly, no editor UI)
	void BuildSceneFromLevel(const LevelData& levelIn, Scene& scene);

	// Validate level data without mutating scene state
	LevelValidationReport ValidateLevelData(const std::string& path, const LevelData& data);

	// Load JSON from path and build the Scene
	bool LoadAndBuild(const std::string& path, Scene& scene);
}
