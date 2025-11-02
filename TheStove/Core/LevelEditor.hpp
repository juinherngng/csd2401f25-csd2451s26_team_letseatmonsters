/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "LevelSerializer.hpp"

class Scene;

class LevelEditor {
public:
	// Load level from 'levelPath' into the given scene (spawns objects, sets defaults).
	bool LoadIntoScene(Scene& scene);

	// Draw the editor ImGui window and apply edits to the scene.
	void DrawUI(Scene& scene);

	// Toggle editor visibility.
	void Toggle() { isEnabled = !isEnabled; }

	// Check if the editor UI is enabled (visible).
	bool IsEnabled() const { return isEnabled; }

	// Set path used by Load/Save.
	void SetPath(const std::string& path) { levelPath = path; }

private:
	bool isEnabled{ true };
	std::string levelPath{};
	LevelData level{};
};
