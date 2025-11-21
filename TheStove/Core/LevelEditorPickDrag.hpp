/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPickDrag.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Picking & dragging helpers for the Level Editor.
					- Convert mouse to scene/world coordinates
					- Pick topmost object under cursor
					- Drag selected object while holding LMB
					- Respect ImGui mouse capture to avoid UI conflicts

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class Scene;
class LevelEditor;

/**
 * @namespace LEPICKDRAG
 * @brief Scene picking/dragging utilities used by the Level Editor.
 */
namespace LEPICKDRAG {
	// Handle scene picking and dragging inside the Scene viewport.
	void HandleScenePickDrag(LevelEditor& editor,
							 Scene& scene,
							 int& selectedIndex,
							 int& selectedObjectId);
}
