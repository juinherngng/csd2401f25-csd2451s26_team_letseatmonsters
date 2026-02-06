/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPickDrag.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

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

#include "Collision.hpp"
#include "LevelEditorPanelLevel.hpp"
#include "LevelEditorPickDrag.hpp"

namespace LEPICKDRAG {
#ifdef _DEBUG

	enum class TransformTool {
		Select,    // Q (no gizmo drag)
		Translate, // internal / legacy
		Scale,     // internal / legacy
		Rotate,	   // E (ring-only)
		Rect       // T (combined move + scale + move arrows)
	};

	enum class GizmoMode {
		Transform, // operate on sprite transform
		Collider   // operate on collider size/offset
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

	struct GizmoColliderBox {
		glm::vec3 centerWorld;  // collider center in world space (x,y,z)
		glm::vec2 sizeWorld;    // full width/height in world units
	};

	// Current active tool (defaults to combined rect gizmo)
	static TransformTool sCurrentTool = TransformTool::Select;

	// Current active axis while dragging via gizmo
	static ActiveAxis sActiveAxis = ActiveAxis::None;

	// Drag start snapshot (when LMB is first pressed)
	static glm::vec2 sDragStartMouseWorld{ 0.f, 0.f };
	static glm::vec3 sDragStartPos{ 0.f, 0.f, 0.f };
	static glm::vec3 sDragStartScale{ 1.f, 1.f, 1.f };
	static float sDragStartRotDeg = 0.f; // in degrees
	static bool sIsRectScaling = false;

	// Mouse screen position at drag start (for pixel threshold)
	static ImVec2 sDragStartMouseScreen{ 0.f, 0.f };
	static bool sRotateDragging = false;

	// Scaling state
	static float sScaleSignX = 1.0f;
	static float sScaleSignY = 1.0f;
	static DragMode sDragMode = DragMode::None;

	// Incremental rotation state (for smooth, continuous rotation)
	static float sLastMouseAngleRad = 0.0f; // last frame angle around pivot
	static float sCurrentRotDegDrag = 0.0f; // accumulated rotation in degrees

	// Which thing the gizmo is currently editing
	static GizmoMode sGizmoMode = GizmoMode::Transform;

	// Collider drag start state (for collider gizmo)
	static glm::vec2 sDragStartColSize{ 0.f, 0.f };
	static glm::vec2 sDragStartColOffset{ 0.f, 0.f };

	// Small helpers
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

		sDragStartColSize = { 0.f, 0.f };
		sDragStartColOffset = { 0.f, 0.f };
	}

	static inline bool IsPointInCircle(ImVec2 p, ImVec2 c, float radius) {
		const float dx = p.x - c.x;
		const float dy = p.y - c.y;
		return (dx * dx + dy * dy) <= radius * radius;
	}

	static inline bool IsPointInRect(ImVec2 p, ImVec2 mn, ImVec2 mx) {
		return (p.x >= mn.x && p.x <= mx.x &&
				p.y >= mn.y && p.y <= mx.y);
	}

	static inline bool IsPointNearLineEnd(ImVec2 p, ImVec2 end, float radius) {
		const float dx = p.x - end.x;
		const float dy = p.y - end.y;
		return (dx * dx + dy * dy) <= radius * radius;
	}

	static void ComputeWorldCorners(const glm::vec3& pos,
									const glm::vec3& scale,
									float rotDeg,
									glm::vec2& outTL,
									glm::vec2& outTR,
									glm::vec2& outBL,
									glm::vec2& outBR) {
		const float hx = 0.5f * scale.x;
		const float hy = 0.5f * scale.y;

		const float rotRad = glm::radians(rotDeg);
		const float c = std::cos(rotRad);
		const float s = std::sin(rotRad);

		auto TransformLocal = [&](float lx, float ly) -> glm::vec2 {
			const float rx = lx * c - ly * s;
			const float ry = lx * s + ly * c;
			return glm::vec2{ pos.x + rx, pos.y + ry };
		};

		// y+ is downwards in your world, so top = -hy, bottom = +hy
		outTL = TransformLocal(-hx, -hy);
		outTR = TransformLocal(hx, -hy);
		outBL = TransformLocal(-hx, hy);
		outBR = TransformLocal(hx, hy);
	}

	static bool GetColliderBoxWorld(GameObject* obj,
									glm::vec3& outCenter,
									glm::vec3& outSize) {
		// Collider size & offset are stored in Math::Vector2D on GameObject
		Math::Vector2D sizeM = obj->GetColliderSize();
		Math::Vector2D offsetM = obj->GetColliderOffset();

		// If collider is not set up, skip
		if (sizeM.x <= 0.0f || sizeM.y <= 0.0f) {
			return false;
		}

		glm::vec3 pos = obj->GetPositionGLM();

		// Center = sprite position + collider offset  (same as DrawBoundingBox)
		outCenter = glm::vec3(
			pos.x + offsetM.x,
			pos.y + offsetM.y,
			pos.z
		);

		// Full size = collider size
		outSize = glm::vec3(
			sizeM.x,
			sizeM.y,
			1.0f
		);

		return true;
	}

	// Exact collider AABB in world space, using the same math as GameObject::DrawBoundingBox
	static bool GetColliderAABBWorld(GameObject* obj,
									 glm::vec2& outTL,
									 glm::vec2& outTR,
									 glm::vec2& outBL,
									 glm::vec2& outBR) {
		// Read collider data
		Math::Vector2D sizeM = obj->GetColliderSize();
		Math::Vector2D offsetM = obj->GetColliderOffset();

		if (sizeM.x <= 0.0f || sizeM.y <= 0.0f) {
			return false;
		}

		glm::vec3 pos = obj->GetPositionGLM();

		// Center = sprite position + offset (same as GameObject::DrawBoundingBox)
		Math::Vector3D centerM(
			pos.x + offsetM.x,
			pos.y + offsetM.y,
			pos.z
		);

		// Scale = collider size
		Math::Vector3D scaleM(sizeM.x, sizeM.y, 1.0f);

		// Use the *exact* collision helper
		collision::AABB box = collision::World::makeAABBFromCenter(centerM, scaleM);

		// In our coordinate system: min.y = top, max.y = bottom
		outTL = glm::vec2(box.min.x, box.min.y); // left,  top
		outTR = glm::vec2(box.max.x, box.min.y); // right, top
		outBL = glm::vec2(box.min.x, box.max.y); // left,  bottom
		outBR = glm::vec2(box.max.x, box.max.y); // right, bottom

		return true;
	}

	// Basis for TRANSFORM gizmo: use sprite transform only
	static void GetTransformBasis(GameObject* obj,
								  glm::vec3& outPos,
								  glm::vec3& outScale,
								  float& outRotDeg) {
		// Use the object's actual transform
		outPos = obj->GetPositionGLM();
		outScale = obj->GetScaleGLM();

		outRotDeg = glm::degrees(obj->GetRotationAngleZ());
		if (!std::isfinite(outRotDeg)) {
			outRotDeg = 0.0f;
		}
	}

	// Basis for COLLIDER gizmo: use collider AABB (no rotation)
	static bool GetColliderBasis(GameObject* obj,
								 glm::vec3& outPos,
								 glm::vec3& outScale,
								 float& outRotDeg) {
		glm::vec3 center{};
		glm::vec3 size{};
		if (!GetColliderBoxWorld(obj, center, size)) {
			return false; // no collider set up
		}

		outPos = center;
		outScale = size;
		outRotDeg = 0.0f; // AABB, no rotation
		return true;
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

		// Tool hotkeys (Q / T / E)
		if (!io.WantCaptureKeyboard) {
			// Q = "no gizmo" / select / direct drag
			if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
				sCurrentTool = TransformTool::Select;
			}

			// T = toggle transform gizmo on/off
			if (ImGui::IsKeyPressed(ImGuiKey_T)) {
				if (sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Rotate) {
					// If a gizmo tool is active, turn it off (back to Q behaviour)
					sCurrentTool = TransformTool::Select;
				}
				else {
					// Turn gizmo on: Rect = move+scale+axis arrows
					sCurrentTool = TransformTool::Rect;
				}
			}

			// E = rotate gizmo (only active while T has turned gizmo "on")
			if (ImGui::IsKeyPressed(ImGuiKey_E)) {
				sCurrentTool = TransformTool::Rotate; // rotation ring only
			}

			// C = toggle between Transform vs Collider gizmo type
			if (ImGui::IsKeyPressed(ImGuiKey_C)) {
				sGizmoMode = (sGizmoMode == GizmoMode::Transform)
					?GizmoMode::Collider
					:GizmoMode::Transform;
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

		// LMB CLICK: start drag or selection
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			sActiveAxis = ActiveAxis::None;
			sDragMode = DragMode::None;
			isDragging = false;
			draggingId = -1;
			sRotateDragging = false;
			sDragStartMouseWorld = mouseWorld;
			sDragStartMouseScreen = mouseScreen;

			bool startedFromGizmo = false;

			// If we already have a selected object, try gizmo interactions first
			if ((sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Rotate) &&
				selectedObjectId >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
				if (sel) {
					glm::vec3 pos{};
					glm::vec3 sz{};
					float rotDeg = 0.0f;

					bool basisOK = false;
					if (sGizmoMode == GizmoMode::Transform) {
						GetTransformBasis(sel, pos, sz, rotDeg);
						basisOK = true;
					}
					else { // Collider gizmo
						basisOK = GetColliderBasis(sel, pos, sz, rotDeg);
					}

					// If collider gizmo requested but collider is missing, do nothing
					if (!basisOK) {
						// let normal picking logic run instead
						goto SKIP_GIZMO_FOR_CLICK;
					}

					glm::vec2 worldCenter{ pos.x, pos.y };
					ImVec2 centerScreen = gfx.WorldToSceneImage(worldCenter);

					// Proper rotated corners in world-space
					glm::vec2 worldTL{}, worldTR{}, worldBL{}, worldBR{};

					if (sGizmoMode == GizmoMode::Transform) {
						// Transform gizmo (Unity-style): sprite bounds, with rotation
						ComputeWorldCorners(pos, sz, rotDeg, worldTL, worldTR, worldBL, worldBR);
					}
					else {
						// Collider gizmo: draw exactly the red collider box
						if (!GetColliderAABBWorld(sel, worldTL, worldTR, worldBL, worldBR)) {
							return; // no collider to draw
						}
					}

					// Convert to screen-space
					ImVec2 bl = gfx.WorldToSceneImage(worldBL);
					ImVec2 br = gfx.WorldToSceneImage(worldBR);
					ImVec2 tl = gfx.WorldToSceneImage(worldTL);
					ImVec2 tr = gfx.WorldToSceneImage(worldTR);

					const float hx = 0.5f * sz.x;
					const float hy = 0.5f * sz.y;

					// Screen-space AABB that matches the yellow rect
					ImVec2 rectMin{
						std::min(std::min(tl.x, tr.x), std::min(bl.x, br.x)),
						std::min(std::min(tl.y, tr.y), std::min(bl.y, br.y))
					};
					ImVec2 rectMax{
						std::max(std::max(tl.x, tr.x), std::max(bl.x, br.x)),
						std::max(std::max(tl.y, tr.y), std::max(bl.y, br.y))
					};

					// World-space body (for body / yellow-box test) using OBB logic
					bool insideBodyWorld = false;
					{
						glm::vec2 local = mouseWorld - glm::vec2{ pos.x, pos.y };

						// Transform mouse into the object's local (unrotated) space
						const float rotRad = glm::radians(rotDeg);
						const float c = std::cos(-rotRad); // inverse rotation
						const float s = std::sin(-rotRad);

						const float lx = local.x * c - local.y * s;
						const float ly = local.x * s + local.y * c;

						insideBodyWorld =
							(lx >= -hx && lx <= hx) &&
							(ly >= -hy && ly <= hy);
					}

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

					// ROTATION RING (E tool)
					if (!startedFromGizmo &&
						sGizmoMode == GizmoMode::Transform &&
						sCurrentTool == TransformTool::Rotate) {
						float radiusWorld = std::max(hx, hy) * 1.3f;
						glm::vec2 worldCirclePoint{ pos.x + radiusWorld, pos.y };
						ImVec2 circleEdgeScreen = gfx.WorldToSceneImage(worldCirclePoint);

						const float dx = circleEdgeScreen.x - centerScreen.x;
						const float dy = circleEdgeScreen.y - centerScreen.y;
						const float radiusScreen = std::sqrt(dx * dx + dy * dy);

						const float circleHitThickness = 16.0f;

						bool onOuter = IsPointInCircle(mouseScreen, centerScreen,
													   radiusScreen + circleHitThickness);
						bool onInner = IsPointInCircle(mouseScreen, centerScreen,
													   radiusScreen - circleHitThickness);

						// Ring is only valid when outside yellow rect but within the annulus
						if (onOuter && !onInner) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

							sDragMode = DragMode::Rotate;
							sActiveAxis = ActiveAxis::XY;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = rotDeg;
							sDragStartMouseWorld = mouseWorld;
							sDragStartMouseScreen = mouseScreen;
							isDragging = true;
							draggingId = selectedObjectId;
							sRotateDragging = false;

							// Initialize incremental rotation state
							glm::vec2 center{ sDragStartPos.x, sDragStartPos.y };
							sLastMouseAngleRad = std::atan2(
								mouseWorld.y - center.y,
								mouseWorld.x - center.x
							);
							sCurrentRotDegDrag = sDragStartRotDeg;

							startedFromGizmo = true;
						}
					}

					// SCALE HANDLES (corners + edges) – Rect tool only
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

						// Corners: uniform XY scaling
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
							sDragStartPos = sel->GetPositionGLM();
							sDragStartScale = sel->GetScaleGLM();
							sDragStartRotDeg = rotDeg;
							sDragStartMouseWorld = mouseWorld;

							// If we're editing the collider, cache its starting size/offset
							if (sGizmoMode == GizmoMode::Collider) {
								Math::Vector2D sizeM = sel->GetColliderSize();
								Math::Vector2D offsetM = sel->GetColliderOffset();
								sDragStartColSize = { sizeM.x, sizeM.y };
								sDragStartColOffset = { offsetM.x, offsetM.y };
							}

							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}

					// MOVE ARROWS (X / Y axis) – Rect tool only
					if (!startedFromGizmo &&
						sGizmoMode == GizmoMode::Transform &&
						sCurrentTool == TransformTool::Rect) {
						const float arrowLenWorld = 64.0f;
						const float handleRadius = 20.0f;

						glm::vec2 worldXEnd{ pos.x + arrowLenWorld, pos.y };
						glm::vec2 worldYEnd{ pos.x, pos.y - arrowLenWorld };

						ImVec2 xEndScreen = gfx.WorldToSceneImage(worldXEnd);
						ImVec2 yEndScreen = gfx.WorldToSceneImage(worldYEnd);

						if (IsPointNearLineEnd(mouseScreen, xEndScreen, handleRadius)) {
							LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

							sDragMode = DragMode::Move;
							sActiveAxis = ActiveAxis::X;
							sDragStartPos = pos;
							sDragStartScale = sz;
							sDragStartRotDeg = rotDeg;
							sDragStartMouseWorld = mouseWorld;
							grabOffset = ImVec2(mouseWorld.x - pos.x, mouseWorld.y - pos.y);
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
							sDragStartRotDeg = rotDeg;
							sDragStartMouseWorld = mouseWorld;
							grabOffset = ImVec2(mouseWorld.x - pos.x, mouseWorld.y - pos.y);
							isDragging = true;
							draggingId = selectedObjectId;
							startedFromGizmo = true;
						}
					}

					// BODY (yellow box) – Rect tool only, lowest priority
					if (!startedFromGizmo &&
						(sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Select) &&
						insideBodyWorld) {

						LEPANELLEVEL::RecordUndoSnapshot(editor, scene);

						sDragMode = DragMode::Move;
						sActiveAxis = ActiveAxis::XY;
						sDragStartPos = sel->GetPositionGLM();
						sDragStartScale = sel->GetScaleGLM();
						sDragStartRotDeg = glm::degrees(sel->GetRotationAngleZ());
						sDragStartMouseWorld = mouseWorld;
						sDragStartMouseScreen = mouseScreen;
						grabOffset = ImVec2(mouseWorld.x - pos.x, mouseWorld.y - pos.y);

						// For collider gizmo, remember starting offset/size
						if (sGizmoMode == GizmoMode::Collider) {
							Math::Vector2D sizeM = sel->GetColliderSize();
							Math::Vector2D offsetM = sel->GetColliderOffset();
							sDragStartColSize = { sizeM.x, sizeM.y };
							sDragStartColOffset = { offsetM.x, offsetM.y };
						}

						isDragging = true;
						draggingId = selectedObjectId;
						sRotateDragging = false;
						startedFromGizmo = true;
					}
				}
			}

		SKIP_GIZMO_FOR_CLICK:
			;

			// If no gizmo handled the click, perform object picking from back to front
			if (!startedFromGizmo) {
				int pickedIndex = -1;

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
						pickedIndex = i;
						break;
					}
				}

				if (pickedIndex >= 0) {
					GameObject* pickedObj = list[pickedIndex];
					selectedObjectId = pickedObj->GetID();
					selectedIndex = -1;

					const glm::vec3 p = pickedObj->GetPositionGLM();
					const glm::vec3 s = pickedObj->GetScaleGLM();
					const float rDeg = glm::degrees(pickedObj->GetRotationAngleZ());

					sDragStartMouseWorld = mouseWorld;
					sDragStartPos = p;
					sDragStartScale = s;
					sDragStartRotDeg = rDeg;
					grabOffset = ImVec2(mouseWorld.x - p.x, mouseWorld.y - p.y);

					if (sCurrentTool == TransformTool::Rect ||
						sCurrentTool == TransformTool::Select) {
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

		// LMB HELD: apply dragging (move / scale / rotate)
		if (isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			GameObject* g = scene.GetGameObjectByID(draggingId);
			if (g) {
				const glm::vec2 delta = mouseWorld - sDragStartMouseWorld;

				glm::vec3 newPos = sDragStartPos;
				glm::vec3 newScale = sDragStartScale;
				float newRotDeg = sDragStartRotDeg;

				// Current collider values
				Math::Vector2D colSize = g->GetColliderSize();
				Math::Vector2D colOffset = g->GetColliderOffset();
				bool colliderChanged = false;

				constexpr float kMinSize = 4.0f;

				switch (sDragMode) {
					case DragMode::Move:
					{
						if (sGizmoMode == GizmoMode::Transform) {
							// Move the object itself
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
						}
						else { // Collider gizmo: move collider offset only
							if (sActiveAxis == ActiveAxis::X) {
								colOffset.x = sDragStartColOffset.x + delta.x;
							}
							else if (sActiveAxis == ActiveAxis::Y) {
								colOffset.y = sDragStartColOffset.y + delta.y;
							}
							else { // XY
								colOffset.x = sDragStartColOffset.x + delta.x;
								colOffset.y = sDragStartColOffset.y + delta.y;
							}
							colliderChanged = true;

							// Keep object where it was
							newPos = sDragStartPos;
							newScale = sDragStartScale;
							newRotDeg = sDragStartRotDeg;
						}
						break;
					}

					case DragMode::Scale:
					{
						if (sGizmoMode == GizmoMode::Transform) {
							// Scale sprite transform (Unity-style transform tool)
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

							const float kScaleDeadZone = 1.0f; // world units
							if (std::abs(newScale.x - sDragStartScale.x) < kScaleDeadZone &&
								std::abs(newScale.y - sDragStartScale.y) < kScaleDeadZone) {
								newScale = sDragStartScale;    // treat as no-scale
							}
						}
						else { // Collider gizmo: resize collider, keep transform
							if (sActiveAxis == ActiveAxis::X) {
								colSize.x = std::max(kMinSize,
													 sDragStartColSize.x + sScaleSignX * delta.x * 2.0f);
								colSize.y = sDragStartColSize.y;
							}
							else if (sActiveAxis == ActiveAxis::Y) {
								colSize.y = std::max(kMinSize,
													 sDragStartColSize.y + sScaleSignY * delta.y * 2.0f);
								colSize.x = sDragStartColSize.x;
							}
							else { // XY
								colSize.x = std::max(kMinSize,
													 sDragStartColSize.x + sScaleSignX * delta.x * 2.0f);
								colSize.y = std::max(kMinSize,
													 sDragStartColSize.y + sScaleSignY * delta.y * 2.0f);
							}

							colliderChanged = true;

							// Do NOT change object transform
							newPos = sDragStartPos;
							newScale = sDragStartScale;
							newRotDeg = sDragStartRotDeg;
						}

						break;
					}

					case DragMode::Rotate:
					{
						if (sCurrentTool == TransformTool::Rotate) {
							// Only start actual rotation after moving a little (pixel threshold)
							const float pixelThreshold = 4.0f;
							float dx = mouseScreen.x - sDragStartMouseScreen.x;
							float dy = mouseScreen.y - sDragStartMouseScreen.y;
							float distSq = dx * dx + dy * dy;

							if (!sRotateDragging) {
								if (distSq < pixelThreshold * pixelThreshold) {
									// treat as click so far (no rotation yet)
									break;
								}

								// exceeded threshold, start rotating
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

							// Rotation sensitivity (tweak as desired)
							constexpr float sensitivity = 0.8f;
							deltaStepDeg *= sensitivity;

							// Accumulate rotation in degrees
							sCurrentRotDegDrag += deltaStepDeg;
							newRotDeg = sCurrentRotDegDrag;

							// Store for next frame
							sLastMouseAngleRad = currentAngle;
						}
						break;
					}

					case DragMode::None:
					default:
					break;
				}

				// Apply any collider edits from collider gizmo
				if (colliderChanged) {
					g->SetColliderSize(colSize);
					g->SetColliderOffset(colOffset);

					// Keep Scene::Defaults in sync for save/load & red debug box
					Scene::Defaults defs = scene.GetDefaults(draggingId);
					defs.colSize = glm::vec2(colSize.x, colSize.y);
					defs.colOff = glm::vec2(colOffset.x, colOffset.y);
					scene.SetDefaults(draggingId, defs);
				}

				scene.SetTransformFromLevel(
					draggingId,
					{ newPos.x, newPos.y, newPos.z },
					{ newScale.x, newScale.y, 1.0f },
					newRotDeg
				);

				// Only clamp object position, not collider edits
				if (sGizmoMode == GizmoMode::Transform &&
					sDragMode == DragMode::Move) {
					scene.ClampToWalkArea(g);
				}
			}
		}

		// LMB RELEASE: stop dragging
		if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			isDragging = false;
			draggingId = -1;
			ResetDragState();
		}

		// Delete key: delete currently selected object
		if (!io.WantCaptureKeyboard &&
			selectedObjectId >= 0 &&
			ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			LEPANELLEVEL::RecordUndoSnapshot(editor, scene);
			scene.DespawnByID(selectedObjectId);

			selectedObjectId = -1;
			selectedIndex = -1;

			isDragging = false;
			draggingId = -1;
			ResetDragState();
		}

		// Gizmo drawing for the selected object
		if (selectedObjectId >= 0 &&
			(sCurrentTool == TransformTool::Rect || sCurrentTool == TransformTool::Rotate)) {
			GameObject* sel = scene.GetGameObjectByID(selectedObjectId);
			if (sel) {
				glm::vec3 pos{};
				glm::vec3 sz{};
				float rotDeg = 0.0f;

				bool basisOK = false;
				if (sGizmoMode == GizmoMode::Transform) {
					GetTransformBasis(sel, pos, sz, rotDeg);
					basisOK = true;
				}
				else { // Collider gizmo
					basisOK = GetColliderBasis(sel, pos, sz, rotDeg);
				}
				if (!basisOK) {
					return; // nothing to draw for collider mode without collider
				}

				// Proper corners in world-space
				glm::vec2 worldTL{}, worldTR{}, worldBL{}, worldBR{};

				if (sGizmoMode == GizmoMode::Transform) {
					// Transform gizmo: use sprite transform / rotation
					ComputeWorldCorners(pos, sz, rotDeg, worldTL, worldTR, worldBL, worldBR);
				}
				else {
					// Collider gizmo: use *exact* collider AABB math (matches red debug box)
					if (!GetColliderAABBWorld(sel, worldTL, worldTR, worldBL, worldBR)) {
						return; // no collider -> nothing to pick
					}
				}

				GraphicsEngine& gfxLocal = GraphicsEngine::Instance();

				ImVec2 bl = gfxLocal.WorldToSceneImage(worldBL);
				ImVec2 br = gfxLocal.WorldToSceneImage(worldBR);
				ImVec2 tl = gfxLocal.WorldToSceneImage(worldTL);
				ImVec2 tr = gfxLocal.WorldToSceneImage(worldTR);

				ImDrawList* dl = ImGui::GetForegroundDrawList();

				// Yellow selection rectangle
				const ImU32 rectCol = IM_COL32(255, 255, 0, 255);
				dl->AddLine(tl, tr, rectCol, 2.0f);
				dl->AddLine(tr, br, rectCol, 2.0f);
				dl->AddLine(br, bl, rectCol, 2.0f);
				dl->AddLine(bl, tl, rectCol, 2.0f);

				// Scale handles (Rect tool only)
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

				// Pivot marker (cyan cross)
				glm::vec2 worldCenter{ pos.x, pos.y };
				ImVec2 centerScreen = gfxLocal.WorldToSceneImage(worldCenter);

				constexpr float pivotSize = 6.0f;
				const ImU32 pivotCol = IM_COL32(0, 255, 255, 255);

				dl->AddLine(ImVec2(centerScreen.x - pivotSize, centerScreen.y),
							ImVec2(centerScreen.x + pivotSize, centerScreen.y),
							pivotCol, 2.0f);
				dl->AddLine(ImVec2(centerScreen.x, centerScreen.y - pivotSize),
							ImVec2(centerScreen.x, centerScreen.y + pivotSize),
							pivotCol, 2.0f);

				// Move arrows (Rect tool only)
				if (sGizmoMode == GizmoMode::Transform &&
					sCurrentTool == TransformTool::Rect) {
					constexpr float kArrowLenWorld = 64.0f;

					glm::vec2 worldXEnd{ pos.x + kArrowLenWorld, pos.y };
					glm::vec2 worldYEnd{ pos.x, pos.y - kArrowLenWorld };

					ImVec2 xEndScreen = gfxLocal.WorldToSceneImage(worldXEnd);
					ImVec2 yEndScreen = gfxLocal.WorldToSceneImage(worldYEnd);

					const ImU32 xCol = IM_COL32(255, 100, 100, 255);
					const ImU32 yCol = IM_COL32(100, 255, 100, 255);

					dl->AddLine(centerScreen, xEndScreen, xCol, 3.0f);
					ImVec2 tipX1{ xEndScreen.x - 6.0f, xEndScreen.y - 4.0f };
					ImVec2 tipX2{ xEndScreen.x - 6.0f, xEndScreen.y + 4.0f };
					dl->AddTriangleFilled(xEndScreen, tipX1, tipX2, xCol);

					dl->AddLine(centerScreen, yEndScreen, yCol, 3.0f);
					ImVec2 tipY1{ yEndScreen.x - 4.0f, yEndScreen.y + 6.0f };
					ImVec2 tipY2{ yEndScreen.x + 4.0f, yEndScreen.y + 6.0f };
					dl->AddTriangleFilled(yEndScreen, tipY1, tipY2, yCol);
				}

				// Rotation circle (Rotate tool only)
				if (sGizmoMode == GizmoMode::Transform &&
					sCurrentTool == TransformTool::Rotate) {
					const float hxLocal = 0.5f * sz.x;
					const float hyLocal = 0.5f * sz.y;

					const float radiusWorld = std::max(hxLocal, hyLocal) * 1.3f;
					glm::vec2  worldCirclePoint{ pos.x + radiusWorld, pos.y };
					ImVec2 circleEdgeScreen = gfxLocal.WorldToSceneImage(worldCirclePoint);

					const float dx = circleEdgeScreen.x - centerScreen.x;
					const float dy = circleEdgeScreen.y - centerScreen.y;
					const float radiusScreen = std::sqrt(dx * dx + dy * dy);

					const ImU32 circleCol = IM_COL32(0, 200, 255, 255);
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
