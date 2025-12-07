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
#ifdef _DEBUG
#include <imgui.h>
#endif
#include <vector>
#include <algorithm> 
#include <cmath>

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/SceneManager.hpp"

#include "LevelEditorPanelLevel.hpp"
#include "LevelEditorPickDrag.hpp"

namespace LEPICKDRAG {
#ifdef _DEBUG

	enum class TransformTool {
		Translate,
		Scale,
		Rotate
	};

	enum class ActiveAxis {
		None,
		X,
		Y,
		XY
	};

	// Current active tool (defaults to move/translate)
	static TransformTool sCurrentTool = TransformTool::Translate;

	// Current active axis while dragging via gizmo
	static ActiveAxis sActiveAxis = ActiveAxis::None;

	// Drag start data – snapshot when LMB is pressed
	static glm::vec2 sDragStartMouseWorld{ 0.f, 0.f };
	static glm::vec3 sDragStartPos{ 0.f, 0.f, 0.f };
	static glm::vec3 sDragStartScale{ 1.f, 1.f, 1.f };
	static float sDragStartRotDeg = 0.f;

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

		// Tool switching: Q = Move, W = Scale, E = Rotate
		if (!io.WantCaptureKeyboard) {
			if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
				sCurrentTool = TransformTool::Translate;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_W)) {
				sCurrentTool = TransformTool::Scale;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_E)) {
				sCurrentTool = TransformTool::Rotate;
			}
		}

		// Small on-screen HUD so you know which gizmo mode is active
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::SetNextWindowPos(ImVec2(10.f, 40.f), ImGuiCond_Always);
		if (ImGui::Begin("TransformToolHUD", nullptr,
						 ImGuiWindowFlags_NoDecoration |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_NoInputs)) {
			const char* modeLabel = "Move";
			switch (sCurrentTool) {
				case TransformTool::Translate: modeLabel = "Move (Q)";   break;
				case TransformTool::Scale: modeLabel = "Scale (W)";  break;
				case TransformTool::Rotate: modeLabel = "Rotate (E)"; break;
			}

			ImGui::Text("Gizmo: %s", modeLabel);
			ImGui::TextDisabled("Q: Move   W: Scale   E: Rotate");
		}

		ImGui::End();

		// Function-scoped statics follow your naming rule: camelCase with leading underscore.
		static bool isDragging = false;
		static int draggingId = -1;
		static ImVec2 grabOffset = ImVec2(0.f, 0.f);

		std::vector<GameObject*> list;
		scene.CollectRenderablePointers(list);

		GraphicsEngine& gfx = GraphicsEngine::Instance();
		ImVec2 mouseScreen = ImGui::GetMousePos();

		auto IsPointInCircle = [](ImVec2 p, ImVec2 c, float radius) {
			float dx = p.x - c.x;
			float dy = p.y - c.y;
			return (dx * dx + dy * dy) <= radius * radius;
		};

		auto IsPointNearLineEnd = [](ImVec2 p, ImVec2 end, float radius) {
			float dx = p.x - end.x;
			float dy = p.y - end.y;
			return (dx * dx + dy * dy) <= radius * radius;
		};

		// LMB click: gizmo hit-test first, then object picking
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			sActiveAxis = ActiveAxis::None;
			bool startedFromGizmo = false;

			// If something is selected, try to start drag from gizmo handles first
			if (selectedObjectId >= 0) {
				GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
				if (sel) {
					const glm::vec3 pos = sel->GetPositionGLM();
					const glm::vec3 sz = sel->GetScaleGLM();

					glm::vec2 worldCenter{ pos.x, pos.y };
					ImVec2 centerScreen = gfx.WorldToSceneImage(worldCenter);

					// Move gizmo: arrows on +X and +Y
					const float arrowLenWorld = 64.0f;
					glm::vec2 worldXEnd{ pos.x + arrowLenWorld, pos.y };
					glm::vec2 worldYEnd{ pos.x, pos.y - arrowLenWorld };

					ImVec2 xEndScreen = gfx.WorldToSceneImage(worldXEnd);
					ImVec2 yEndScreen = gfx.WorldToSceneImage(worldYEnd);

					// Rotate gizmo: circle around object (radius from bbox)
					float hx = 0.5f * sz.x;
					float hy = 0.5f * sz.y;
					float radiusWorld = std::max(hx, hy) * 1.3f;
					glm::vec2 worldCirclePoint{ pos.x + radiusWorld, pos.y };
					ImVec2 circleEdgeScreen = gfx.WorldToSceneImage(worldCirclePoint);
					float radiusScreen = std::sqrt(
						(circleEdgeScreen.x - centerScreen.x) * (circleEdgeScreen.x - centerScreen.x) +
						(circleEdgeScreen.y - centerScreen.y) * (circleEdgeScreen.y - centerScreen.y));

					// Decide based on current tool
					if (sCurrentTool == TransformTool::Translate) {
						// Prioritise axis handles over free-move
						const float handleRadius = 12.0f;
						if (IsPointNearLineEnd(mouseScreen, xEndScreen, handleRadius)) {
							// X-axis move
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sActiveAxis = ActiveAxis::X;
							sDragStartMouseWorld = mouseWorld;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							grabOffset = ImVec2(mouseWorld.x - pos.x, mouseWorld.y - pos.y);
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
						else if (IsPointNearLineEnd(mouseScreen, yEndScreen, handleRadius)) {
							// Y-axis move
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sActiveAxis = ActiveAxis::Y;
							sDragStartMouseWorld = mouseWorld;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							grabOffset = ImVec2(mouseWorld.x - pos.x, mouseWorld.y - pos.y);
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}
					else if (sCurrentTool == TransformTool::Rotate) {
						// Click near rotation circle to start rotation
						const float circleHitThickness = 10.0f;
						if (IsPointInCircle(mouseScreen, centerScreen, radiusScreen + circleHitThickness) &&
							!IsPointInCircle(mouseScreen, centerScreen, radiusScreen - circleHitThickness)) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sActiveAxis = ActiveAxis::XY;
							sDragStartMouseWorld = mouseWorld;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}
					else if (sCurrentTool == TransformTool::Scale) {
						// Corner-only scaling – only react if clicking a yellow handle

						// Compute corner positions in screen space
						glm::vec2 worldBL{ pos.x - hx, pos.y + hy }; // bottom-left
						glm::vec2 worldBR{ pos.x + hx, pos.y + hy };
						glm::vec2 worldTL{ pos.x - hx, pos.y - hy };
						glm::vec2 worldTR{ pos.x + hx, pos.y - hy };

						ImVec2 bl = gfx.WorldToSceneImage(worldBL);
						ImVec2 br = gfx.WorldToSceneImage(worldBR);
						ImVec2 tl = gfx.WorldToSceneImage(worldTL);
						ImVec2 tr = gfx.WorldToSceneImage(worldTR);

						const float handleSize = 6.0f;
						const float hit = handleSize + 3.0f; // slightly bigger hitbox

						ImVec2 tlMin{ tl.x - hit, tl.y - hit };
						ImVec2 tlMax{ tl.x + hit, tl.y + hit };

						ImVec2 trMin{ tr.x - hit, tr.y - hit };
						ImVec2 trMax{ tr.x + hit, tr.y + hit };

						ImVec2 blMin{ bl.x - hit, bl.y - hit };
						ImVec2 blMax{ bl.x + hit, bl.y + hit };

						ImVec2 brMin{ br.x - hit, br.y - hit };
						ImVec2 brMax{ br.x + hit, br.y + hit };

						auto IsPointInRect = [](ImVec2 p, ImVec2 mn, ImVec2 mx) {
							return (p.x >= mn.x && p.x <= mx.x &&
									p.y >= mn.y && p.y <= mx.y);
						};

						if (IsPointInRect(mouseScreen, tlMin, tlMax) ||
							IsPointInRect(mouseScreen, trMin, trMax) ||
							IsPointInRect(mouseScreen, blMin, blMax) ||
							IsPointInRect(mouseScreen, brMin, brMax)) {
							// Start uniform-ish scaling from any corner
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sActiveAxis = ActiveAxis::XY;
							sDragStartMouseWorld = mouseWorld;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}
				}
			}

			// If not clicking on gizmo, fall back to picking topmost object and starting drag
			if (!startedFromGizmo) {
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
					selectedIndex = -1; // will be recomputed in the Level panel

					LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

					GameObject* pickedObj = list[picked];
					const glm::vec3 p = pickedObj->GetPositionGLM();
					const glm::vec3 s = pickedObj->GetScaleGLM();
					const float     rDeg = glm::degrees(pickedObj->GetRotationAngleZ());

					// Store drag-start state for all tools
					sDragStartMouseWorld = mouseWorld;
					sDragStartPos = p;
					sDragStartScale = s;
					sDragStartRotDeg = rDeg;

					// Default: free movement / scaling / rotation
					sActiveAxis = ActiveAxis::XY;

					// Offset only used for translate/move gizmo
					grabOffset = ImVec2(mouseWorld.x - p.x, mouseWorld.y - p.y);

					if (sCurrentTool == TransformTool::Scale) {
						// In Scale mode: clicking the body only selects (no drag)
						isDragging = false;
						draggingId = -1;
					}
					else {
						// In Move / Rotate: click body and drag immediately
						isDragging = true;
						draggingId = selectedObjectId;
					}
				}
			}
		}

		// LMB hold: apply current gizmo (Move / Scale / Rotate) to the selected object
		if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			GameObject* g = scene.GetGameObjectByID(draggingId);
			if (g) {
				// Delta from drag start, in world space
				const glm::vec2 delta = mouseWorld - sDragStartMouseWorld;

				glm::vec3 newPos = sDragStartPos;
				glm::vec3 newScale = sDragStartScale;
				float newRotDeg = sDragStartRotDeg;

				constexpr float kMinSize = 4.0f;
				constexpr float kSnapMove = 8.0f; // position snap step
				constexpr float kSnapRot = 15.0f; // rotation snap step

				switch (sCurrentTool) {
					case TransformTool::Translate:
					{
						// Default free-move keeps the grab point under the cursor
						if (sActiveAxis == ActiveAxis::X) {
							newPos.x = sDragStartPos.x + delta.x;
						}
						else if (sActiveAxis == ActiveAxis::Y) {
							newPos.y = sDragStartPos.y + delta.y;
						}
						else { // free XY
							newPos.x = mouseWorld.x - grabOffset.x;
							newPos.y = mouseWorld.y - grabOffset.y;
						}

						// Optional snapping with Shift (Unity-like)
						if (io.KeyShift) {
							newPos.x = std::round(newPos.x / kSnapMove) * kSnapMove;
							newPos.y = std::round(newPos.y / kSnapMove) * kSnapMove;
						}
						break;
					}
					case TransformTool::Scale:
					{
						if (sActiveAxis == ActiveAxis::X) {
							newScale.x = std::max(kMinSize, sDragStartScale.x + delta.x * 2.0f);
							newScale.y = sDragStartScale.y;
						}
						else if (sActiveAxis == ActiveAxis::Y) {
							newScale.y = std::max(kMinSize, sDragStartScale.y - delta.y * 2.0f);
							newScale.x = sDragStartScale.x;
						}
						else { // free uniform-ish scaling
							newScale.x = std::max(kMinSize, sDragStartScale.x + delta.x * 2.0f);
							newScale.y = std::max(kMinSize, sDragStartScale.y - delta.y * 2.0f);
						}
						break;
					}
					case TransformTool::Rotate:
					{
						// Rotate around the object centre using mouse angle
						glm::vec2 center{ sDragStartPos.x, sDragStartPos.y };

						float startAngle = std::atan2(
							sDragStartMouseWorld.y - center.y,
							sDragStartMouseWorld.x - center.x);

						float currentAngle = std::atan2(
							mouseWorld.y - center.y,
							mouseWorld.x - center.x);

						float deltaAngleDeg = glm::degrees(currentAngle - startAngle);
						newRotDeg = sDragStartRotDeg + deltaAngleDeg;

						// Snap with Shift (e.g., 15-degree increments)
						if (io.KeyShift) {
							newRotDeg = std::round(newRotDeg / kSnapRot) * kSnapRot;
						}
						break;
					}
				}

				scene.SetTransformFromLevel(
					draggingId,
					{ newPos.x, newPos.y, newPos.z },
					{ newScale.x, newScale.y, 1.0f },
					newRotDeg
				);

				scene.ClampToWalkArea(g);
			}
		}

		// LMB release: stop dragging
		if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isDragging = false;
			draggingId = -1;
			sActiveAxis = ActiveAxis::None;
		}

		// Delete key for selected object
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
			sActiveAxis = ActiveAxis::None;
		}

		// Gizmo overlay drawing
		if (selectedObjectId >= 0) {
			GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
			if (sel) {
				// Position and size in world space
				const glm::vec3 pos = sel->GetPositionGLM();
				const glm::vec3 sz = sel->GetScaleGLM();
				const float hx = 0.5f * sz.x;
				const float hy = 0.5f * sz.y;

				// Four world-space corners of the sprite rect
				glm::vec2 worldBL{ pos.x - hx, pos.y + hy }; // bottom-left (Y-down ref)
				glm::vec2 worldBR{ pos.x + hx, pos.y + hy };
				glm::vec2 worldTL{ pos.x - hx, pos.y - hy };
				glm::vec2 worldTR{ pos.x + hx, pos.y - hy };

				// Convert to screen-space inside the Scene image
				ImVec2 bl = gfx.WorldToSceneImage(worldBL);
				ImVec2 br = gfx.WorldToSceneImage(worldBR);
				ImVec2 tl = gfx.WorldToSceneImage(worldTL);
				ImVec2 tr = gfx.WorldToSceneImage(worldTR);

				ImDrawList* dl = ImGui::GetForegroundDrawList();

				// Selection rectangle
				const ImU32 rectCol = IM_COL32(255, 255, 0, 255); // yellow
				dl->AddLine(tl, tr, rectCol, 2.0f);
				dl->AddLine(tr, br, rectCol, 2.0f);
				dl->AddLine(br, bl, rectCol, 2.0f);
				dl->AddLine(bl, tl, rectCol, 2.0f);

				// Corner "handles" (purely visual for scale)
				const float handleSize = 6.0f;
				const ImU32 handleCol = IM_COL32(255, 200, 0, 255);

				auto DrawHandle = [&](ImVec2 p) {
					ImVec2 a{ p.x - handleSize, p.y - handleSize };
					ImVec2 b{ p.x + handleSize, p.y + handleSize };
					dl->AddRectFilled(a, b, handleCol);
				};

				DrawHandle(tl);
				DrawHandle(tr);
				DrawHandle(bl);
				DrawHandle(br);

				// Pivot marker at centre
				glm::vec2 worldCenter{ pos.x, pos.y };
				ImVec2 centerScreen = gfx.WorldToSceneImage(worldCenter);
				const float pivotSize = 6.0f;
				ImU32 pivotCol = IM_COL32(0, 255, 255, 255);
				dl->AddLine(ImVec2(centerScreen.x - pivotSize, centerScreen.y),
							ImVec2(centerScreen.x + pivotSize, centerScreen.y), pivotCol, 2.0f);
				dl->AddLine(ImVec2(centerScreen.x, centerScreen.y - pivotSize),
							ImVec2(centerScreen.x, centerScreen.y + pivotSize), pivotCol, 2.0f);

				// Tool-specific gizmo overlay
				if (sCurrentTool == TransformTool::Translate) {
					// Move gizmo: X (red) and Y (green) arrows
					const float arrowLenWorld = 64.0f;
					glm::vec2 worldXEnd{ pos.x + arrowLenWorld, pos.y };
					glm::vec2 worldYEnd{ pos.x, pos.y - arrowLenWorld };

					ImVec2 xEndScreen = gfx.WorldToSceneImage(worldXEnd);
					ImVec2 yEndScreen = gfx.WorldToSceneImage(worldYEnd);

					ImU32 xCol = IM_COL32(255, 100, 100, 255);
					ImU32 yCol = IM_COL32(100, 255, 100, 255);

					// X-axis arrow
					dl->AddLine(centerScreen, xEndScreen, xCol, 3.0f);
					// little triangle tip
					ImVec2 tipX1{ xEndScreen.x - 6.0f, xEndScreen.y - 4.0f };
					ImVec2 tipX2{ xEndScreen.x - 6.0f, xEndScreen.y + 4.0f };
					dl->AddTriangleFilled(xEndScreen, tipX1, tipX2, xCol);

					// Y-axis arrow
					dl->AddLine(centerScreen, yEndScreen, yCol, 3.0f);
					ImVec2 tipY1{ yEndScreen.x - 4.0f, yEndScreen.y + 6.0f };
					ImVec2 tipY2{ yEndScreen.x + 4.0f, yEndScreen.y + 6.0f };
					dl->AddTriangleFilled(yEndScreen, tipY1, tipY2, yCol);
				}
				else if (sCurrentTool == TransformTool::Rotate) {
					// Rotation circle gizmo
					float radiusWorld = std::max(hx, hy) * 1.3f;
					glm::vec2 worldCirclePoint{ pos.x + radiusWorld, pos.y };
					ImVec2 circleEdgeScreen = gfx.WorldToSceneImage(worldCirclePoint);
					float radiusScreen = std::sqrt(
						(circleEdgeScreen.x - centerScreen.x) * (circleEdgeScreen.x - centerScreen.x) +
						(circleEdgeScreen.y - centerScreen.y) * (circleEdgeScreen.y - centerScreen.y));

					ImU32 circleCol = IM_COL32(0, 200, 255, 255);
					dl->AddCircle(centerScreen, radiusScreen, circleCol, 64, 2.0f);

					// Small rotation handle above top edge (kept from original)
					ImVec2 topMid{ 0.5f * (tl.x + tr.x), 0.5f * (tl.y + tr.y) - 18.0f };
					dl->AddCircleFilled(topMid, 4.0f, circleCol);
					dl->AddLine(ImVec2(0.5f * (tl.x + tr.x), 0.5f * (tl.y + tr.y)), topMid, circleCol, 2.0f);
				}
			}
		}
	}
#else
	// Release build: no-op stub so code can link but no ImGui is referenced.
	void HandleScenePickDrag(LevelEditor& /*editor*/,
							 Scene& /*scene*/,
							 int& /*selectedIndex*/,
							 int& /*selectedObjectId*/) {
		// Intentionally empty in Release builds (editor UI disabled).
	}
#endif
}
