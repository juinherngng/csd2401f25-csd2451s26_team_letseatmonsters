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
#ifdef _DEBUG
	namespace {
		// Helper function to draw a full-width button with a callback.
		inline void DrawActionButton(const char* label, const std::function<void()>& callback) {
			if (ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f)) && callback) {
				callback();
			}
		}

		// Helper function to draw a full-width button that can be disabled based on the 'disabled' flag.
		inline void DrawActionButtonDisabled(const char* label, bool disabled, const std::function<void()>& callback) {
			ImGui::BeginDisabled(disabled);
			DrawActionButton(label, callback);
			ImGui::EndDisabled();
		}
	}
#endif

	// Handles undo/redo shortcuts (Ctrl+Z / Ctrl+Y or Cmd+Z / Cmd+Shift+Z) and calls the provided callbacks if the shortcuts are triggered.
	// Only active when not playing and when ImGui is not capturing keyboard input.
	void HandleUndoRedoShortcuts(LevelEditor& editor, const std::function<bool()>& undo, const std::function<bool()>& redo) {
#ifndef _DEBUG
		(void)editor;
		(void)undo;
		(void)redo;
#else
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
#ifndef _DEBUG
		(void)editor;
		(void)callbacks;
#else
		if (ImGui::BeginTable("##LevelActionsGrid", 4, ImGuiTableFlags_SizingStretchSame)) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			DrawActionButton("Load Level", callbacks.onLoad);

			ImGui::TableSetColumnIndex(1);
			DrawActionButton("New Scene", callbacks.onNewScene);

			ImGui::TableSetColumnIndex(2);
			DrawActionButton("Save Level", callbacks.onSave);

			ImGui::TableSetColumnIndex(3);
			DrawActionButtonDisabled("Play", editor.IsPlaying(), callbacks.onPlay);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			DrawActionButtonDisabled("Undo", editor.IsPlaying(), callbacks.onUndo);

			ImGui::TableSetColumnIndex(1);
			DrawActionButtonDisabled("Redo", editor.IsPlaying(), callbacks.onRedo);

			ImGui::TableSetColumnIndex(2);
			DrawActionButtonDisabled("Stop", !editor.IsPlaying(), callbacks.onStop);

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
