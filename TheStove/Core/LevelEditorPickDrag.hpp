/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPickDrag.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Header for picking and dragging utilities in the Level Editor, which handle
					mouse input to select and manipulate scene objects in the Scene viewport.

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
