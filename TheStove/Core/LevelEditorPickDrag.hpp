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

// Forward declare GameObject to avoid circular dependency.
namespace LEPICKDRAG {
	// Handle scene picking and dragging inside the Scene viewport.
	void HandleScenePickDrag(LevelEditor& editor,
		Scene& scene,
		int& selectedIndex,
		int& selectedObjectId);
}
