/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorActions.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements the UI actions for the level editor, including undo/redo shortcuts
					and action buttons for loading, saving, and controlling simulation playback.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
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

		/**
		 * @brief Draws a full-width toolbar button and invokes its callback when clicked.
		 * @param label Button label.
		 * @param callback Callback to invoke after a click.
		 */
		inline void DrawActionButton(const char* label, const std::function<void()>& callback) {
			if (ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f)) && callback) {
				callback();
			}
		}

		/**
		 * @brief Draws a full-width toolbar button with optional disabled state.
		 * @param label Button label.
		 * @param disabled True to disable interaction with the button.
		 * @param callback Callback to invoke after a click.
		 */
		inline void DrawActionButtonDisabled(const char* label, bool disabled, const std::function<void()>& callback) {
			ImGui::BeginDisabled(disabled);
			DrawActionButton(label, callback);
			ImGui::EndDisabled();
		}
	}
#endif

	/**
	 * @brief Processes editor undo and redo shortcuts.
	 * @param editor Shared level editor controller.
	 * @param undo Callback used when an undo shortcut is triggered.
	 * @param redo Callback used when a redo shortcut is triggered.
	 */
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

	/**
	 * @brief Draws the main action grid for file, history, and simulation controls.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param callbacks Callback bundle invoked by the toolbar buttons.
	 */
	void DrawActionGrid(LevelEditor& editor, Scene& scene, const Callbacks& callbacks) {
		(void)scene;
#ifndef _DEBUG
		(void)editor;
		(void)callbacks;
#else
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));

		if (ImGui::BeginTable("##LevelActionsGroups", 3, ImGuiTableFlags_SizingStretchSame)) {
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextDisabled("File");
			DrawActionButton("Load", callbacks.onLoad);
			DrawActionButton("New", callbacks.onNewScene);
			DrawActionButton("Save", callbacks.onSave);

			ImGui::TableSetColumnIndex(1);
			ImGui::TextDisabled("History");
			DrawActionButtonDisabled("Undo", editor.IsPlaying(), callbacks.onUndo);
			DrawActionButtonDisabled("Redo", editor.IsPlaying(), callbacks.onRedo);

			ImGui::TableSetColumnIndex(2);
			ImGui::TextDisabled("Simulation");

			const bool isPlaying = editor.IsPlaying();
			const bool isSimActive = scene.IsSimulationActive();

			if (!isPlaying) {
				const ImVec4 primary = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
				ImGui::PushStyleColor(ImGuiCol_Button, primary);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, primary);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, primary);
				DrawActionButton("Play", callbacks.onPlay);
				ImGui::PopStyleColor(3);
			}
			else {
				DrawActionButtonDisabled("Play", true, callbacks.onPlay);
			}

			DrawActionButtonDisabled("Stop", !isPlaying, callbacks.onStop);

			const char* pauseLabel = isSimActive ? "Pause" : "Resume";
			ImGui::BeginDisabled(!isPlaying);
			if (ImGui::Button(pauseLabel, ImVec2(-FLT_MIN, 0.0f))) {
				scene.SetSimulationActive(!isSimActive);
			}

			ImGui::EndDisabled();

			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
#endif
	}
}

