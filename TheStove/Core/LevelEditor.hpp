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

/**
 * @class LevelEditor
 * @brief Hosts all ImGui panels for level editing (Level, Prefabs, Assets, Inspector).
 *
 * Notes:
 * - JSON/editor rotation is in DEGREES.
 * - GameObject setters expect RADIANS (convert at call site).
 */
class LevelEditor {
public:
	// ----- Lifecycle -----
	LevelEditor() = default;
	~LevelEditor() = default;

	// Draw all editor UI (Level, Prefabs, Assets); also handles picking/dragging when not playing
	void DrawUI(Scene& scene);

	// Load current levelPath into the scene. Returns false if load fails.
	bool LoadIntoScene(Scene& scene);

	// ----- Controls & Path -----
	bool IsEnabled() const;
	void Toggle();
	void SetPath(const std::string& path);

	// Current JSON file used for loading/saving levels.
	std::string levelPath{};

private:
	// State flags
	bool isEnabled = true;
	bool isPlaying = false;

	// Data models
	LevelData level{};			   // Working copy for editing (Save/Load)
	LevelData playStartSnapshot{}; // Snapshot captured at Play, restored on Stop
};
