/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorActions.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the UI actions for the level editor, including undo/redo shortcuts
					and action buttons for loading, saving, and controlling simulation playback.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorActions.hpp"

#include <cfloat>

#ifdef _DEBUG
#include <imgui.h>
#endif

namespace LEACTIONS {
	// Handles undo/redo shortcuts (Ctrl+Z / Ctrl+Y or Cmd+Z / Cmd+Shift+Z) and calls the provided callbacks if the shortcuts are triggered.
	// Only active when not playing and when ImGui is not capturing keyboard input.
	void HandleUndoRedoShortcuts(LevelEditor& editor, const std::function<bool()>& undo, const std::function<bool()>& redo) {
#ifdef _DEBUG
		ImGuiIO& io = ImGui::GetIO();
		if (!editor.IsPlaying() && !io.WantCaptureKeyboard && (io.KeyCtrl || io.KeySuper) && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
			if (undo) {
				undo();
			}
		}

		if (!editor.IsPlaying() && !io.WantCaptureKeyboard && (io.KeyCtrl || io.KeySuper) && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) {
			if (redo) {
				redo();
			}
		}
#endif
	}

	// Renders the action grid with buttons for Load, New Scene, Save, Undo, Redo, Play, Stop, and Pause/Resume.
	// Buttons are disabled based on the editor's play state. Calls the provided callbacks when buttons are clicked.
	void DrawActionGrid(LevelEditor& editor, Scene& scene, const Callbacks& callbacks) {
		(void)scene;
#ifdef _DEBUG
		if (ImGui::BeginTable("##LevelActionsGrid", 4, ImGuiTableFlags_SizingStretchSame)) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Load Level", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onLoad) {
				callbacks.onLoad();
			}

			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("New Scene", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onNewScene) {
				callbacks.onNewScene();
			}

			ImGui::TableSetColumnIndex(2);
			if (ImGui::Button("Save Level", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onSave) {
				callbacks.onSave();
			}

			ImGui::TableSetColumnIndex(3);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Undo", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onUndo) {
				callbacks.onUndo();
			}

			ImGui::EndDisabled();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Redo", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onRedo) {
				callbacks.onRedo();
			}

			ImGui::EndDisabled();
			ImGui::TableSetColumnIndex(1);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Play", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onPlay) {
				callbacks.onPlay();
			}

			ImGui::EndDisabled();
			ImGui::TableSetColumnIndex(2);
			ImGui::BeginDisabled(!editor.IsPlaying());
			if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0.0f)) && callbacks.onStop) {
				callbacks.onStop();
			}

			ImGui::EndDisabled();
			ImGui::TableSetColumnIndex(3);
			const bool isSimActive = scene.IsSimulationActive();
			const char* pauseLabel = isSimActive ? "Pause" : "Resume";
			ImGui::BeginDisabled(!editor.IsPlaying());
			if (ImGui::Button(pauseLabel, ImVec2(-FLT_MIN, 0.0f))) {
				scene.SetSimulationActive(!isSimActive);
			}

			ImGui::EndDisabled();
			ImGui::EndTable();
		}
#endif
	}
}
