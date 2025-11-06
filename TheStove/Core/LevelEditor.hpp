/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		In-engine level editor panel:
					- Load/Save JSON levels
					- Object hierarchy + property inspector
					- Drag/drop textures & prefabs
					- Play/Stop snapshot restore

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <unordered_map>

#include "LevelSerializer.hpp"
#include "InputManager.hpp"

class Scene;

class LevelEditor {
public:
	// Lifecycle / Windows
	LevelEditor() = default;
	~LevelEditor() = default;

	// Draw the full Level/Prefabs/Assets UI and handle interactions.
	void DrawUI(Scene& scene);

	// Load current levelPath into the scene. Returns false if load failed.
	bool LoadIntoScene(Scene& scene);

	// Controls / Path
	bool IsEnabled() const;
	void Toggle();
	void SetPath(const std::string& path);

	std::string levelPath{};

private:
	bool isEnabled = true;
	bool isPlaying = false;

	LevelData level{};			   // Working copy for editing (Save/Load)
	LevelData playStartSnapshot{}; // Snapshot captured at Play, restored on Stop
};
