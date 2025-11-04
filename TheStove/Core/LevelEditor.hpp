/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		In-engine level editor interface. Rotation values are handled in DEGREES
					at the editor/JSON layer; conversions to RADIANS should happen at the
					GameObject boundary (see LevelEditor.cpp usage).

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <unordered_map>

#include "LevelSerializer.hpp"
#include "InputManager.hpp"
#include "../Graphics/SceneManager.hpp"

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

	// Play/Stop state 
	bool isPlaying = false;
	LevelData playStartSnapshot;

	// Prefab links
	std::unordered_map<int, std::string> prefabPathById;

private:
	bool isEnabled{ true };
	std::string levelPath{};
	LevelData level{};
};
