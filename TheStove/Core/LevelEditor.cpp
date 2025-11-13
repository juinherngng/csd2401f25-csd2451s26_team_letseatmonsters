/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Simple in-engine Level Editor window.
					- JSON/Editor store rotation in DEGREES
					- GameObject setters receive RADIANS (convert at call-site)
					- Delegates to Level/Prefabs/Assets panels
					- Disables pick/drag while playing

		 All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditor.hpp"

#include <imgui.h>

#include "LevelEditorPickDrag.hpp"
#include "LevelEditorPanelLevel.hpp"
#include "LevelEditorPanelPrefabs.hpp"
#include "LevelEditorPanelAssets.hpp"

#include "InputManager.hpp"

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"

 // DrawUI
void LevelEditor::DrawUI(Scene& scene) {
	ImGuiStyle& st = ImGui::GetStyle();
	st.FrameRounding = 3;
	st.FramePadding = ImVec2(5, 3);
	st.ItemSpacing = ImVec2(6, 4);
	st.WindowPadding = ImVec2(10, 10);

	// Panels maintain these between frames; static matches your existing behavior
	static int selectedIndex = -1;  // index in hierarchy list
	static int selectedObjectId = -1;  // engine object ID

	// The scene viewport should not capture game mouse by default while drawing editor UI
	InputManager::Get().SetSceneViewportWantsGameMouse(false);

	if (!isEnabled) {
		return;
	}

	// Defensive: skip if called outside an ImGui frame
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (ctx == nullptr || !ctx->WithinFrameScope) {
		return;
	}

	// ----- Panels -----
	LEPANELLEVEL::DrawLevelPanel(*this, scene, selectedIndex, selectedObjectId);
	LEPANELPREFABS::DrawPrefabsPanel(*this, scene, selectedObjectId);
	LEPANELASSETS::DrawAssetsPanel(*this, scene, selectedIndex, selectedObjectId);

	// Disable editor picking/dragging while the game is running
	if (!isPlaying) {
		LEPICKDRAG::HandleScenePickDrag(*this, scene, selectedIndex, selectedObjectId);
	}
}
