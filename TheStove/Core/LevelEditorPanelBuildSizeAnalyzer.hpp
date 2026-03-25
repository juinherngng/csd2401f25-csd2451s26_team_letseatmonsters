/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditorPanelBuildSizeAnalyzer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Build Size Analyzer panel for the Level Editor.
					- Lists exportable project assets with file sizes
					- Supports per-asset selection and select all / deselect all
					- Tracks total selected size live
					- Exports selected assets to a chosen folder

		 All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

class LevelEditor;
class Scene;

namespace LEPANELBUILDSIZEANALYZER {
	/**
	 * @brief Draws the Build Size Analyzer panel.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 */
	void DrawBuildSizeAnalyzerPanel(LevelEditor& editor, Scene& scene);
}
