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
		Select,    // Q (no gizmo drag)
		Translate, // internal / legacy
		Scale,     // internal / legacy
		Rotate,	   // internal / legacy
		Rect       // T (combined move + scale)
	};

	enum class DragMode {
		None,
		Move,
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
	static TransformTool sCurrentTool = TransformTool::Rect;

	// Current active axis while dragging via gizmo
	static ActiveAxis sActiveAxis = ActiveAxis::None;

	// Drag start data – snapshot when LMB is pressed
	static glm::vec2 sDragStartMouseWorld{ 0.f, 0.f };
	static glm::vec3 sDragStartPos{ 0.f, 0.f, 0.f };
	static glm::vec3 sDragStartScale{ 1.f, 1.f, 1.f };
	static float sDragStartRotDeg = 0.f;
	static bool sIsRectScaling = false;

	static ImVec2 sDragStartMouseScreen{ 0.f, 0.f };
	static bool sRotateDragging = false;

	static float sScaleSignX = 1.0f;
	static float sScaleSignY = 1.0f;
	static DragMode sDragMode = DragMode::None;

	static float sLastMouseAngleRad = 0.0f;
	static float sCurrentRotDegDrag = 0.0f;

	static void ResetDragState() {
		sActiveAxis = ActiveAxis::None;
		sDragMode = DragMode::None;
		sDragStartMouseWorld = { 0.f, 0.f };
		sDragStartPos = { 0.f, 0.f, 0.f };
		sDragStartScale = { 1.f, 1.f, 1.f };
		sDragStartRotDeg = 0.f;

		sDragStartMouseScreen = ImVec2(0.f, 0.f);
		sRotateDragging = false;

		sLastMouseAngleRad = 0.0f;
		sCurrentRotDegDrag = 0.0f;
	}

	void HandleScenePickDrag(LevelEditor& editor,
							 Scene& scene,
							 int& selectedIndex,
							 int& selectedObjectId) {
		// Convert mouse coordinates into world space inside the Scene image
		glm::vec2 mouseWorld{};
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			return;
		}

		ImGuiIO& io = ImGui::GetIO();

		// Q = Select, T = Rect (move + scale), E = Rotate (ring-only)
		if (!io.WantCaptureKeyboard) {
			if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
				sCurrentTool = TransformTool::Select;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_T)) {
				// "move/scale" tool (no rotation)
				sCurrentTool = TransformTool::Rect;
			}
			if (ImGui::IsKeyPressed(ImGuiKey_E)) {
				// "rotate" tool (ring only)
				sCurrentTool = TransformTool::Rotate;
			}
		}

		// Persistent drag state
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

		auto IsPointInRect = [](ImVec2 p, ImVec2 mn, ImVec2 mx) {
			return (p.x >= mn.x && p.x <= mx.x &&
					p.y >= mn.y && p.y <= mx.y);
		};

		auto IsPointNearLineEnd = [](ImVec2 p, ImVec2 end, float radius) {
			float dx = p.x - end.x;
			float dy = p.y - end.y;
			return (dx * dx + dy * dy) <= radius * radius;
		};

		// LMB CLICK: start drag
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			sActiveAxis = ActiveAxis::None;
			sDragMode = DragMode::None;
			isDragging = false;
			draggingId = -1;
			sRotateDragging = false;
			sDragStartMouseWorld = mouseWorld;
			sDragStartMouseScreen = mouseScreen;

			bool startedFromGizmo = false;

			// Only show gizmo in combined tool, not in Select
			if ((sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Rotate) &&
				selectedObjectId >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
				if (sel) {
					const glm::vec3 pos = sel->GetPositionGLM();
					const glm::vec3 sz = sel->GetScaleGLM();

					glm::vec2 worldCenter{ pos.x, pos.y };
					ImVec2 centerScreen = gfx.WorldToSceneImage(worldCenter);

					const float hx = 0.5f * sz.x;
					const float hy = 0.5f * sz.y;

					// Basic rect corners in world space
					glm::vec2 worldBL{ pos.x - hx, pos.y + hy };
					glm::vec2 worldBR{ pos.x + hx, pos.y + hy };
					glm::vec2 worldTL{ pos.x - hx, pos.y - hy };
					glm::vec2 worldTR{ pos.x + hx, pos.y - hy };

					ImVec2 bl = gfx.WorldToSceneImage(worldBL);
					ImVec2 br = gfx.WorldToSceneImage(worldBR);
					ImVec2 tl = gfx.WorldToSceneImage(worldTL);
					ImVec2 tr = gfx.WorldToSceneImage(worldTR);

					// Screen-space AABB that matches the yellow rect
					ImVec2 rectMin{
						std::min(std::min(tl.x, tr.x), std::min(bl.x, br.x)),
						std::min(std::min(tl.y, tr.y), std::min(bl.y, br.y))
					};
					ImVec2 rectMax{
						std::max(std::max(tl.x, tr.x), std::max(bl.x, br.x)),
						std::max(std::max(tl.y, tr.y), std::max(bl.y, br.y))
					};

					// World-space body AABB for the selected object
					bool insideBodyWorld =
						(mouseWorld.x >= pos.x - hx && mouseWorld.x <= pos.x + hx) &&
						(mouseWorld.y >= pos.y - hy && mouseWorld.y <= pos.y + hy);

					// Midpoints for edge handles
					ImVec2 topMid{
						0.5f * (tl.x + tr.x),
						0.5f * (tl.y + tr.y)
					};
					ImVec2 bottomMid{
						0.5f * (bl.x + br.x),
						0.5f * (bl.y + br.y)
					};
					ImVec2 leftMid{
						0.5f * (tl.x + bl.x),
						0.5f * (tl.y + bl.y)
					};
					ImVec2 rightMid{
						0.5f * (tr.x + br.x),
						0.5f * (tr.y + br.y)
					};

					// ROTATION RING
					if (!startedFromGizmo && sCurrentTool == TransformTool::Rotate) {
						float radiusWorld = std::max(hx, hy) * 1.3f;
						glm::vec2 worldCirclePoint{ pos.x + radiusWorld, pos.y };
						ImVec2 circleEdgeScreen = gfx.WorldToSceneImage(worldCirclePoint);

						float radiusScreen = std::sqrt(
							(circleEdgeScreen.x - centerScreen.x) * (circleEdgeScreen.x - centerScreen.x) +
							(circleEdgeScreen.y - centerScreen.y) * (circleEdgeScreen.y - centerScreen.y));

						const float circleHitThickness = 10.0f;

						bool onOuter = IsPointInCircle(mouseScreen, centerScreen,
													   radiusScreen + circleHitThickness);
						bool onInner = IsPointInCircle(mouseScreen, centerScreen,
													   radiusScreen - circleHitThickness);

						bool insideRectScreen = IsPointInRect(mouseScreen, rectMin, rectMax);

						if (!insideRectScreen && onOuter && !onInner) {
							// Clicked on ring: prepare rotation drag (no rotation until movement)
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sDragMode = DragMode::Rotate;
							sActiveAxis = ActiveAxis::XY;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							sDragStartMouseWorld = mouseWorld;
							sDragStartMouseScreen = mouseScreen;
							isDragging = true;
							draggingId = selectedObjectId;
							sRotateDragging = false;

							// initialize incremental rotation state
							glm::vec2 center{ sDragStartPos.x, sDragStartPos.y };
							sLastMouseAngleRad = std::atan2(
								mouseWorld.y - center.y,
								mouseWorld.x - center.x
							);
							sCurrentRotDegDrag = sDragStartRotDeg;

							startedFromGizmo = true;
						}
					}

					// SCALE HANDLES (corners + edges)
					if (!startedFromGizmo && sCurrentTool == TransformTool::Rect) {
						const float handleSize = 6.0f;
						const float hitSize = handleSize + 3.0f;

						auto MakeBox = [&](ImVec2 c) {
							ImVec2 mn{ c.x - hitSize, c.y - hitSize };
							ImVec2 mx{ c.x + hitSize, c.y + hitSize };
							return std::pair<ImVec2, ImVec2>(mn, mx);
						};

						auto [tlMin, tlMax] = MakeBox(tl);
						auto [trMin, trMax] = MakeBox(tr);
						auto [blMin, blMax] = MakeBox(bl);
						auto [brMin, brMax] = MakeBox(br);
						auto [topMin, topMax] = MakeBox(topMid);
						auto [bottomMin, bottomMax] = MakeBox(bottomMid);
						auto [leftMin, leftMax] = MakeBox(leftMid);
						auto [rightMin, rightMax] = MakeBox(rightMid);

						// Corners: uniform XY
						if (IsPointInRect(mouseScreen, tlMin, tlMax)) {
							sScaleSignX = -1.0f;
							sScaleSignY = -1.0f;
							sActiveAxis = ActiveAxis::XY;
						}
						else if (IsPointInRect(mouseScreen, trMin, trMax)) {
							sScaleSignX = 1.0f;
							sScaleSignY = -1.0f;
							sActiveAxis = ActiveAxis::XY;
						}
						else if (IsPointInRect(mouseScreen, blMin, blMax)) {
							sScaleSignX = -1.0f;
							sScaleSignY = 1.0f;
							sActiveAxis = ActiveAxis::XY;
						}
						else if (IsPointInRect(mouseScreen, brMin, brMax)) {
							sScaleSignX = 1.0f;
							sScaleSignY = 1.0f;
							sActiveAxis = ActiveAxis::XY;
						}
						// Edges: X or Y only
						else if (IsPointInRect(mouseScreen, leftMin, leftMax)) {
							sScaleSignX = -1.0f;
							sScaleSignY = 1.0f; // unused
							sActiveAxis = ActiveAxis::X;
						}
						else if (IsPointInRect(mouseScreen, rightMin, rightMax)) {
							sScaleSignX = 1.0f;
							sScaleSignY = 1.0f; // unused
							sActiveAxis = ActiveAxis::X;
						}
						else if (IsPointInRect(mouseScreen, topMin, topMax)) {
							sScaleSignX = 1.0f; // unused
							sScaleSignY = -1.0f;
							sActiveAxis = ActiveAxis::Y;
						}
						else if (IsPointInRect(mouseScreen, bottomMin, bottomMax)) {
							sScaleSignX = 1.0f; // unused
							sScaleSignY = 1.0f;
							sActiveAxis = ActiveAxis::Y;
						}

						if (sActiveAxis != ActiveAxis::None) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sDragMode = DragMode::Scale;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							sDragStartMouseWorld = mouseWorld;
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}

					// MOVE ARROWS (X / Y axis)
					if (!startedFromGizmo && sCurrentTool == TransformTool::Rect) {
						const float arrowLenWorld = 64.0f;
						glm::vec2 worldXEnd{ pos.x + arrowLenWorld, pos.y };
						glm::vec2 worldYEnd{ pos.x, pos.y - arrowLenWorld };

						ImVec2 xEndScreen = gfx.WorldToSceneImage(worldXEnd);
						ImVec2 yEndScreen = gfx.WorldToSceneImage(worldYEnd);

						const float handleRadius = 20.0f;

						if (IsPointNearLineEnd(mouseScreen, xEndScreen, handleRadius)) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sDragMode = DragMode::Move;
							sActiveAxis = ActiveAxis::X;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							sDragStartMouseWorld = mouseWorld;
							grabOffset = ImVec2(mouseWorld.x - pos.x,
												mouseWorld.y - pos.y);
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
						else if (IsPointNearLineEnd(mouseScreen, yEndScreen, handleRadius)) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
							sDragMode = DragMode::Move;
							sActiveAxis = ActiveAxis::Y;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
							sDragStartMouseWorld = mouseWorld;
							grabOffset = ImVec2(mouseWorld.x - pos.x,
												mouseWorld.y - pos.y);
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}

					// If nothing else (ring/handles/arrows) claimed the click, body move wins
					if (!startedFromGizmo &&
						sCurrentTool == TransformTool::Rect &&
						insideBodyWorld) {

						LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

						sDragMode = DragMode::Move;
						sActiveAxis = ActiveAxis::XY;
						sDragStartPos = pos;
						sDragStartScale = sz;
						sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
						sDragStartMouseWorld = mouseWorld;
						sDragStartMouseScreen = mouseScreen;
						grabOffset = ImVec2(mouseWorld.x - pos.x,
											mouseWorld.y - pos.y);

						isDragging = true;
						draggingId = selectedObjectId;
						sRotateDragging = false;
						startedFromGizmo = true;
					}
				}
			}

			// FALLBACK: pick object from body click
			if (!startedFromGizmo) {
				int picked = -1;

				for (int i = static_cast<int>(list.size()) - 1; i >= 0; --i) {
					GameObject* g = list[i];
					if (!g) continue;

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
					selectedObjectId = list[picked]->GetID();
					selectedIndex = -1;

					GameObject* pickedObj = list[picked];
					const glm::vec3 p = pickedObj->GetPositionGLM();
					const glm::vec3 s = pickedObj->GetScaleGLM();
					const float     rDeg = glm::degrees(pickedObj->GetRotationAngleZ());

					sDragStartMouseWorld = mouseWorld;
					sDragStartPos = p;
					sDragStartScale = s;
					sDragStartRotDeg = rDeg;
					grabOffset = ImVec2(mouseWorld.x - p.x,
										mouseWorld.y - p.y);

					// If in Rect tool, clicking body starts Move
					if (sCurrentTool == TransformTool::Rect) {
						LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
						sDragMode = DragMode::Move;
						sActiveAxis = ActiveAxis::XY;
						isDragging = true;
						draggingId = selectedObjectId;
					}
					else {
						sDragMode = DragMode::None;
						isDragging = false;
						draggingId = -1;
					}
				}
			}
		}

		// LMB HOLD: apply transform
		if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			GameObject* g = scene.GetGameObjectByID(draggingId);
			if (g) {
				const glm::vec2 delta = mouseWorld - sDragStartMouseWorld;

				glm::vec3 newPos = sDragStartPos;
				glm::vec3 newScale = sDragStartScale;
				// Default: keep current object rotation
				float newRotDeg = sDragStartRotDeg;

				constexpr float kMinSize = 4.0f;

				switch (sDragMode) {
					case DragMode::Move:
					{
						if (sActiveAxis == ActiveAxis::X) {
							newPos.x = sDragStartPos.x + delta.x;
						}
						else if (sActiveAxis == ActiveAxis::Y) {
							newPos.y = sDragStartPos.y + delta.y;
						}
						else {
							newPos.x = mouseWorld.x - grabOffset.x;
							newPos.y = mouseWorld.y - grabOffset.y;
						}
						break;
					}
					case DragMode::Scale:
					{
						if (sActiveAxis == ActiveAxis::X) {
							newScale.x = std::max(kMinSize,
												  sDragStartScale.x + sScaleSignX * delta.x * 2.0f);
						}
						else if (sActiveAxis == ActiveAxis::Y) {
							newScale.y = std::max(kMinSize,
												  sDragStartScale.y + sScaleSignY * delta.y * 2.0f);
						}
						else { // XY
							newScale.x = std::max(kMinSize,
												  sDragStartScale.x + sScaleSignX * delta.x * 2.0f);
							newScale.y = std::max(kMinSize,
												  sDragStartScale.y + sScaleSignY * delta.y * 2.0f);
						}
						break;
					}
					case DragMode::Rotate:
					{
						if (sCurrentTool == TransformTool::Rotate) {
							// Only start actual rotation after mouse moved enough
							const float pixelThreshold = 12.0f;
							float dx = mouseScreen.x - sDragStartMouseScreen.x;
							float dy = mouseScreen.y - sDragStartMouseScreen.y;
							float distSq = dx * dx + dy * dy;

							if (!sRotateDragging) {
								if (distSq < pixelThreshold * pixelThreshold) {
									// treat as click so far (no rotation yet)
									break;
								}
								sRotateDragging = true;
							}

							glm::vec2 center{ sDragStartPos.x, sDragStartPos.y };

							// angle of current mouse position around pivot
							float currentAngle = std::atan2(
								mouseWorld.y - center.y,
								mouseWorld.x - center.x);

							// incremental angle step since last frame, wrapped to [-pi, pi]
							float deltaStep = currentAngle - sLastMouseAngleRad;
							const float pi = 3.14159265358979323846f;
							if (deltaStep > pi) {
								deltaStep -= 2.0f * pi;
							}
							else if (deltaStep < -pi) {
								deltaStep += 2.0f * pi;
							}

							float deltaStepDeg = glm::degrees(deltaStep);

							// rotation sensitivity (you can tweak this)
							float sensitivity = 0.8f;
							deltaStepDeg *= sensitivity;

							// accumulate rotation in degrees
							sCurrentRotDegDrag += deltaStepDeg;
							newRotDeg = sCurrentRotDegDrag;

							// store for next frame
							sLastMouseAngleRad = currentAngle;
						}
						break;
					}
					case DragMode::None:
					default:
					break;
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

		// LMB RELEASE: stop dragging
		if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isDragging = false;
			draggingId = -1;
			sActiveAxis = ActiveAxis::None;
			sDragMode = DragMode::None;
			sRotateDragging = false;
		}

		// Delete key to remove selected
		if (!io.WantCaptureKeyboard &&
			selectedObjectId >= 0 &&
			ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
			scene.DespawnByID(selectedObjectId);
			selectedObjectId = -1;
			selectedIndex = -1;
			isDragging = false;
			draggingId = -1;
			sActiveAxis = ActiveAxis::None;
			sDragMode = DragMode::None;
			sRotateDragging = false;
		}

		// Gizmo drawing (combined)
		if (selectedObjectId >= 0 &&
			(sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Rotate)) {
			GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
			if (sel) {
				const glm::vec3 pos = sel->GetPositionGLM();
				const glm::vec3 sz = sel->GetScaleGLM();

				const float hx = 0.5f * sz.x;
				const float hy = 0.5f * sz.y;

				glm::vec2 worldBL{ pos.x - hx, pos.y + hy };
				glm::vec2 worldBR{ pos.x + hx, pos.y + hy };
				glm::vec2 worldTL{ pos.x - hx, pos.y - hy };
				glm::vec2 worldTR{ pos.x + hx, pos.y - hy };

				ImVec2 bl = gfx.WorldToSceneImage(worldBL);
				ImVec2 br = gfx.WorldToSceneImage(worldBR);
				ImVec2 tl = gfx.WorldToSceneImage(worldTL);
				ImVec2 tr = gfx.WorldToSceneImage(worldTR);

				ImDrawList* dl = ImGui::GetForegroundDrawList();

				const ImU32 rectCol = IM_COL32(255, 255, 0, 255);
				dl->AddLine(tl, tr, rectCol, 2.0f);
				dl->AddLine(tr, br, rectCol, 2.0f);
				dl->AddLine(br, bl, rectCol, 2.0f);
				dl->AddLine(bl, tl, rectCol, 2.0f);

				// Scale handles — Rect tool only
				if (sCurrentTool == TransformTool::Rect) {
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

					ImVec2 topMid{
						0.5f * (tl.x + tr.x),
						0.5f * (tl.y + tr.y)
					};
					ImVec2 bottomMid{
						0.5f * (bl.x + br.x),
						0.5f * (bl.y + br.y)
					};
					ImVec2 leftMid{
						0.5f * (tl.x + bl.x),
						0.5f * (tl.y + bl.y)
					};
					ImVec2 rightMid{
						0.5f * (tr.x + br.x),
						0.5f * (tr.y + br.y)
					};

					DrawHandle(topMid);
					DrawHandle(bottomMid);
					DrawHandle(leftMid);
					DrawHandle(rightMid);
				}

				// Pivot marker
				glm::vec2 worldCenter{ pos.x, pos.y };
				ImVec2 centerScreen = gfx.WorldToSceneImage(worldCenter);
				const float pivotSize = 6.0f;
				ImU32 pivotCol = IM_COL32(0, 255, 255, 255);
				dl->AddLine(ImVec2(centerScreen.x - pivotSize, centerScreen.y),
							ImVec2(centerScreen.x + pivotSize, centerScreen.y),
							pivotCol, 2.0f);
				dl->AddLine(ImVec2(centerScreen.x, centerScreen.y - pivotSize),
							ImVec2(centerScreen.x, centerScreen.y + pivotSize),
							pivotCol, 2.0f);

				// Move arrows — Rect tool only
				if (sCurrentTool == TransformTool::Rect) {
					const float arrowLenWorld = 64.0f;
					glm::vec2 worldXEnd{ pos.x + arrowLenWorld, pos.y };
					glm::vec2 worldYEnd{ pos.x, pos.y - arrowLenWorld };

					ImVec2 xEndScreen = gfx.WorldToSceneImage(worldXEnd);
					ImVec2 yEndScreen = gfx.WorldToSceneImage(worldYEnd);

					ImU32 xCol = IM_COL32(255, 100, 100, 255);
					ImU32 yCol = IM_COL32(100, 255, 100, 255);

					dl->AddLine(centerScreen, xEndScreen, xCol, 3.0f);
					ImVec2 tipX1{ xEndScreen.x - 6.0f, xEndScreen.y - 4.0f };
					ImVec2 tipX2{ xEndScreen.x - 6.0f, xEndScreen.y + 4.0f };
					dl->AddTriangleFilled(xEndScreen, tipX1, tipX2, xCol);

					dl->AddLine(centerScreen, yEndScreen, yCol, 3.0f);
					ImVec2 tipY1{ yEndScreen.x - 4.0f, yEndScreen.y + 6.0f };
					ImVec2 tipY2{ yEndScreen.x + 4.0f, yEndScreen.y + 6.0f };
					dl->AddTriangleFilled(yEndScreen, tipY1, tipY2, yCol);
				}

				// Rotation circle — Rotate tool only
				if (sCurrentTool == TransformTool::Rotate) {
					float radiusWorld = std::max(hx, hy) * 1.3f;
					glm::vec2 worldCirclePoint{ pos.x + radiusWorld, pos.y };
					ImVec2 circleEdgeScreen = gfx.WorldToSceneImage(worldCirclePoint);
					float radiusScreen = std::sqrt(
						(circleEdgeScreen.x - centerScreen.x) * (circleEdgeScreen.x - centerScreen.x) +
						(circleEdgeScreen.y - centerScreen.y) * (circleEdgeScreen.y - centerScreen.y));
					ImU32 circleCol = IM_COL32(0, 200, 255, 255);
					dl->AddCircle(centerScreen, radiusScreen, circleCol, 64, 2.0f);
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
