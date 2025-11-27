/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         RuntimeLevel.hpp
 PROJECT NAME:      Project GAM200

 DESCRIPTION:       Runtime utilities for loading a JSON level and building a Scene (no ImGui).
 ----------------------------------------------------------------------------------------------------
*/
#pragma once

#include <string>
#include "LevelSerializer.hpp"

class Scene;

namespace RuntimeLevel {
	// Build the current Scene from LevelData (runtime-friendly, no editor UI)
	void BuildSceneFromLevel(const LevelData& levelIn, Scene& scene);

	// Load JSON from path and build the Scene
	bool LoadAndBuild(const std::string& path, Scene& scene);
}
