/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPickDrag.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Implementation of picking/dragging behavior for the Level Editor.
					- Converts mouse to world-space (via GraphicsEngine)
					- Picks topmost sprite hit by the cursor
					- Drags while LMB is held, updates Scene transform
					- Stops when LMB is released
					- Early-out when ImGui wants mouse to prevent UI conflict

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <vector>

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/SceneManager.hpp"

#include "LevelEditorPanelLevel.hpp"
#include "LevelEditorPickDrag.hpp"

namespace LEPICKDRAG {
	void HandleScenePickDrag(LevelEditor& editor,
							 Scene& scene,
							 int& selectedIndex,
							 int& selectedObjectId) {
		// Convert mouse coordinates into world space inside the Scene image
		glm::vec2 mouseWorld{};
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			return;
		}

		// ImGui IO for keyboard checks (used later for Delete key)
		ImGuiIO& io = ImGui::GetIO();

		// Function-scoped statics follow your naming rule: camelCase with leading underscore.
		static bool isDragging = false;
		static int draggingId = -1;
		static ImVec2 grabOffset = ImVec2(0.f, 0.f);

		std::vector<GameObject*> list;
		scene.CollectRenderablePointers(list);

		// LMB click: pick topmost object under the cursor and start dragging
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			int picked = -1;

			// Find topmost (last drawn) by iterating from back to front
			for (int i = static_cast<int>(list.size()) - 1; i >= 0; --i) {
				GameObject* g = list[i];
				if (!g) {
					continue;
				}

				const glm::vec3 pos = g->GetPositionGLM();
				const glm::vec3 sz = g->GetScaleGLM();

				const float hx = 0.5f * sz.x;
				const float hy = 0.5f * sz.y;

				const bool inside =
					(mouseWorld.x >= pos.x - hx && mouseWorld.x <= pos.x + hx) &&
					(mouseWorld.y >= pos.y - hy && mouseWorld.y <= pos.y + hy);

				if (inside) {
					picked = i;
					break;
				}
			}

			if (picked >= 0) {
				// Drive selection by object ID
				selectedObjectId = list[picked]->GetID();
				selectedIndex = -1;   // will be recomputed in the Level panel

				LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

				const glm::vec3 p = list[picked]->GetPositionGLM();
				grabOffset = ImVec2(mouseWorld.x - p.x, mouseWorld.y - p.y);

				isDragging = true;
				draggingId = selectedObjectId;
			}
		}

		// LMB hold: move (drag) the selected object
		if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			GameObject* g = scene.GetGameObjectByID(draggingId);
			if (g) {
				const float newX = mouseWorld.x - grabOffset.x;
				const float newY = mouseWorld.y - grabOffset.y;

				const glm::vec3 size = g->GetScaleGLM();
				const float rotDeg = glm::degrees(g->GetRotationAngleZ());

				scene.SetTransformFromLevel(
					draggingId,
					{ newX, newY, g->GetPositionGLM().z },
					{ size.x, size.y, 1.f },
					rotDeg
				);

				scene.ClampToWalkArea(g);
			}
		}

		// LMB release: stop dragging
		if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isDragging = false;
			draggingId = -1;
		}

		if (!io.WantCaptureKeyboard &&
			selectedObjectId >= 0 &&
			ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

			// Remove the object from the scene
			scene.DespawnByID(selectedObjectId);

			// Clear selection and drag state so inspector & editor are clean
			selectedObjectId = -1;
			selectedIndex = -1;
			isDragging = false;
			draggingId = -1;
		}
	}
}
