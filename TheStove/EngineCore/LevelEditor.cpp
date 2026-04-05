/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu		(80%)
 CO-AUTHOR:			Seah Wang Hua, wanghua.seah"digipen.edu (20%)

 DESCRIPTION:		Simple in-engine Level Editor window.
					- JSON/Editor store rotation in DEGREES
					- GameObject setters receive RADIANS (convert at call-site)
					- Delegates to Level/Prefabs/Assets panels
					- Disables pick/drag while playing

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <imgui.h>
#include <imgui_internal.h>

#include "EngineCore/InputManager.hpp"
#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorPanelAssets.hpp"
#include "EngineCore/LevelEditorPanelAudioControl.hpp"
#include "EngineCore/LevelEditorPanelBuildSizeAnalyzer.hpp"
#include "EngineCore/LevelEditorPanelConfig.hpp"
#include "EngineCore/LevelEditorPanelFonts.hpp"
#include "EngineCore/LevelEditorPanelLevel.hpp"
#include "EngineCore/LevelEditorPanelPrefabs.hpp"
#include "EngineCore/LevelEditorPickDrag.hpp"

 /**
  * @brief Draws all top-level editor panels for the current ImGui frame.
  * @param scene Scene currently being edited.
  */
void LevelEditor::DrawUI(Scene& scene) {
	// Fast path: editor disabled
	if (!isEnabled) {
		return;
	}

	// Defensive: skip if called outside an ImGui frame or when ImGui not initialized.
	// Must check before calling any ImGui functions (ImGui::GetStyle(), etc.).
	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (ctx == nullptr || !ctx->WithinFrameScope) {
		return;
	}

	// Now safe to call ImGui APIs
	ImGuiStyle& st = ImGui::GetStyle();
	st.FrameRounding = 3;
	st.FramePadding = ImVec2(6, 4);
	st.ItemSpacing = ImVec2(8, 6);
	st.WindowPadding = ImVec2(10, 10);
	st.CellPadding = ImVec2(6, 4);

	// Keep the current selection stable across frames so all editor panels stay synchronized.
	static int selectedIndex = -1;    // index in hierarchy list
	static int selectedObjectId = -1; // engine object ID

	// The scene viewport should not capture game mouse by default while drawing editor UI
	InputManager::Get().SetSceneViewportWantsGameMouse(false);

	// Draw each dockable editor panel using the shared editor state above.
	LEPANELLEVEL::DrawLevelPanel(*this, scene, selectedIndex, selectedObjectId);
	LEPANELPREFABS::DrawPrefabsPanel(*this, scene, selectedObjectId);
	LEPANELASSETS::DrawAssetsPanel(*this, scene, selectedIndex, selectedObjectId);
	LEPANELAUDIOCONTROL::DrawAudioControlPanel(*this, scene);
	LEPANELCONFIG::DrawConfigPanel(*this, scene);
	LEPANELFONTS::DrawFontsPanel(*this, scene);
	LEPANELBUILDSIZEANALYZER::DrawBuildSizeAnalyzerPanel(*this, scene);

	// Disable editor picking/dragging while the game is running
	if (!isPlaying) {
		LEPICKDRAG::HandleScenePickDrag(*this, scene, selectedIndex, selectedObjectId);
	}
}
