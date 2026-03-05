/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeLevel.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements RuntimeLevel utilities to parse LevelData JSON, spawn animated/static GameObjects
					with proper layers/tags/colliders/animations, set scene backgrounds, rebuild colliders, and
					store object metadata for runtime level loading. (For use outside of editor only.)

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */
#pragma once

#include "LevelSerializer.hpp"

#include <string>

class Scene;

namespace RuntimeLevel {
	// Build the current Scene from LevelData (runtime-friendly, no editor UI)
	void BuildSceneFromLevel(const LevelData& levelIn, Scene& scene);

	// Load JSON from path and build the Scene
	bool LoadAndBuild(const std::string& path, Scene& scene);
}
