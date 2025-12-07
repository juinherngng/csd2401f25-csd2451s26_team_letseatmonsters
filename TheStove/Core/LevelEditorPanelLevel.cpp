/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelLevel.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu
 CO-AUTHOR:			Seah Wang Hua, wanghua.seah"@digipen.edu

 DESCRIPTION:       Implementation of the Level panel.
					- Load/Save levels to JSON
					- Play/Stop scene simulation
					- Hierarchy list and object inspector
					- Add/Remove objects
					- Drag-drop prefab/texture instantiation
					- Keeps LevelData synchronized with Scene state

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#ifdef _DEBUG
#include <imgui.h>
#include <imgui_internal.h>
#endif

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/Layer.hpp"

#include "LevelEditorPanelLevel.hpp"
#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "LevelEditorPrefabLinks.hpp"
#include "InputManager.hpp"


namespace fs = std::filesystem;

using namespace LEFILEIO;

namespace {
	// Internal helpers for Level <-> Scene synchronization
	void SyncLevelToScene(const LevelData& levelIn, Scene& scene);
	void SyncSceneToLevel(Scene& scene, LevelData& levelOut);

#ifdef _DEBUG
	static constexpr int MAX_UNDO = 50;
	static std::vector<LevelData> sUndoStack;

	// Take a snapshot of the current Scene into LevelData and push onto the stack.
	static void PushUndoSnapshot(LevelEditor& editor, Scene& scene) {
		LevelData snap{};
		SyncSceneToLevel(scene, snap);

		sUndoStack.push_back(snap);
		if (sUndoStack.size() > MAX_UNDO) {
			sUndoStack.erase(sUndoStack.begin());
		}

		// Keep the editor's working LevelData in sync with the scene
		editor.MutableLevel() = snap;
	}

	// Pop last snapshot and restore it into the Scene.
	static bool PerformUndo(LevelEditor& editor, Scene& scene) {
		if (sUndoStack.empty()) {
			return false;
		}

		LevelData snap = sUndoStack.back();
		sUndoStack.pop_back();

		scene.ClearAll();
		SyncLevelToScene(snap, scene);
		scene.RebuildColliders();
		scene.SetSimulationActive(false);

		editor.SetPlaying(false);
		editor.MutableLevel() = snap;

		return true;
	}
#endif // _DEBUG

	// Build the current scene from loaded LevelData.
	void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
		for (const auto& obj : levelIn.objects) {
			GameObject* g = nullptr;

			// Use "Default" when the saved layer name is empty
			std::string layerName = obj.layer.empty()?"1":obj.layer;

			// Spawn animated or static
			if (obj.animated) {
				const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
				g = scene.SpawnAnimatedSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h },
											  fullFrame, 0.25f, true, layerName);

				if (obj.texture.find("dino") != std::string::npos) {
					scene.AttachDinoAnimations(g->GetID());

					const std::string clip = obj.animName.empty()?"IDLE":obj.animName;
					scene.SetAnimation(g->GetID(), clip);
				}
			}
			else {
				g = scene.SpawnStaticSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h }, layerName);
			}

			if (!g) {
				std::cerr << "Spawn failed: " << obj.texture << std::endl;
				continue;
			}

			float rotDeg = obj.rotation;

			// Clean up any old bad data that was saved previously
			if (!std::isfinite(rotDeg)) {
				rotDeg = 0.0f;
			}

			// Optional: keep it within [0, 360) if you want
			rotDeg = std::fmod(rotDeg, 360.0f);
			if (rotDeg < 0.0f) rotDeg += 360.0f;

			// Apply to object
			g->SetRotation(glm::radians(rotDeg), { 0, 0, 1 });

			// Collider data
			g->SetColliderSize({ obj.colWidth, obj.colHeight });
			g->SetColliderOffset({ obj.colOffsetX, obj.colOffsetY });

			// Fallback collider if missing
			if (obj.colWidth <= 0.f || obj.colHeight <= 0.f) {
				const glm::vec3 s = g->GetScaleGLM();
				g->SetColliderSize({ s.x, s.y });
				g->SetColliderOffset({ 0.f, 0.f });
			}

			// Track texture path
			scene.SetObjectTexturePath(g->GetID(), obj.texture);

			// Tag-based special IDs and velocity
			if (obj.tag == "player") {
				scene.SetPlayerID(g->GetID());
			}
			else if (obj.tag == "npc1") {
				scene.SetNPC1ID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}
			else if (obj.tag == "npc2") {
				scene.SetNPC2ID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}
			else if (obj.tag == "dino") {
				scene.SetDinoID(g->GetID());
				scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);
			}

			// Store transform and defaults
			scene.SetTransformFromLevel(g->GetID(), { obj.x, obj.y, 0.0f }, { obj.w, obj.h, 1.0f }, obj.rotation);

			Scene::Defaults defs{};
			defs.pos = { obj.x, obj.y, 0.0f };
			defs.size = { obj.w, obj.h };
			defs.rot = obj.rotation;
			defs.colSize = { obj.colWidth, obj.colHeight };
			defs.colOff = { obj.colOffsetX, obj.colOffsetY };
			defs.vel = { obj.speedX, obj.speedY };
			defs.texture = obj.texture;
			defs.tag = obj.tag;
			defs.layer = obj.layer;
			// NEW: approach offset
			defs.approachOffset = { obj.approachOffsetX, obj.approachOffsetY };

			scene.SetDefaults(g->GetID(), defs);
			scene.AttachLogicForTag(g->GetID(), obj.tag);
			scene.ClampToWalkArea(g);
		}
	}

	// Build LevelData snapshot from the current scene.
	void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
		levelOut.objects.clear();

		std::vector<GameObject*> list = scene.GetAllObjectsRaw();

		for (GameObject* g : list) {
			if (!g) {
				continue;
			}

			LevelObject out{};
			out.texture = scene.GetObjectTexturePath(g->GetID());
			out.animated = scene.HasAnimations(g->GetID());
			out.animName = scene.GetCurrentAnimationName(g->GetID());
			out.layer = scene.GetObjectLayer(g->GetID());

			const glm::vec3 p = g->GetPositionGLM();
			const glm::vec3 s = g->GetScaleGLM();

			out.x = p.x; out.y = p.y; out.z = p.z;
			out.w = s.x; out.h = s.y;

			float rotDeg = glm::degrees(g->GetRotationAngleZ());
			if (!std::isfinite(rotDeg)) {
				rotDeg = 0.0f; // clamp broken angles
			}

			out.rotation = rotDeg;

			const auto csz = g->GetColliderSize();
			const auto cof = g->GetColliderOffset();
			out.colWidth = csz.x; out.colHeight = csz.y;
			out.colOffsetX = cof.x; out.colOffsetY = cof.y;

			if (g->GetID() == scene.GetPlayerID()) {
				out.tag = "player";
			}
			else if (g->GetID() == scene.GetNPC1ID()) {
				out.tag = "npc1";
			}
			else if (g->GetID() == scene.GetNPC2ID()) {
				out.tag = "npc2";
			}
			else if (g->GetID() == scene.GetDinoID()) {
				out.tag = "dino";
			}
			else {
				// Preserve whatever tag was originally assigned in the level/defaults
				Scene::Defaults defs = scene.GetDefaults(g->GetID());
				out.tag = defs.tag;  // this will be "table", "customer_table", "work_table", etc.
				out.approachOffsetX = defs.approachOffset.x;
				out.approachOffsetY = defs.approachOffset.y;
			}


			const glm::vec2 v = scene.GetNPCVelocity(g->GetID());
			out.speedX = v.x; out.speedY = v.y;

			levelOut.objects.push_back(out);
		}
	}

#ifdef _DEBUG
	// Draws the advanced Layering System UI (debug-only)
	static void DrawLayerManager(Scene& scene, int selectedObjectId) {
		if (!ImGui::CollapsingHeader("Layering System", ImGuiTreeNodeFlags_DefaultOpen)) {
			return;
		}

		// New layer creation
		ImGui::TextUnformatted("Create a new layer:");
		static char newLayerBuf[64] = "";
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		ImGui::InputText("##NewLayerName", newLayerBuf, IM_ARRAYSIZE(newLayerBuf));
		ImGui::SameLine();

		if (ImGui::Button("Add Layer") && newLayerBuf[0] != '\0') {
			scene.AddLayer(newLayerBuf);
			newLayerBuf[0] = '\0';
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Existing layers:");
		ImGui::Separator();

		const auto& layerMap = scene.GetAllLayers();
		if (layerMap.empty()) {
			ImGui::TextDisabled("No layers yet. Objects fall back to \"Default\".");
			return;
		}

		// Copy & sort by name for stable display
		std::vector<std::pair<std::string, const Layer*>> sorted;
		sorted.reserve(layerMap.size());
		for (const auto& pair : layerMap) {
			sorted.emplace_back(pair.first, &pair.second);
		}

		for (const auto& entry : sorted) {
			const std::string& layerName = entry.first;
			Layer* layer = scene.GetLayer(layerName);
			if (!layer) {
				continue;
			}

			ImGui::PushID(layerName.c_str());

			bool visible = layer->IsVisible();
			bool collidable = layer->IsCollidable();
			const int count = static_cast<int>(layer->GetObjects().size());

			// Layer name
			ImGui::TextUnformatted(layerName.c_str());
			ImGui::SameLine(180.0f);

			// Visible checkbox
			if (ImGui::Checkbox("Visible", &visible)) {
				layer->SetVisible(visible);
			}

			ImGui::SameLine();

			// Collisions checkbox
			if (ImGui::Checkbox("Collisions", &collidable)) {
				layer->SetCollidable(collidable);
			}

			ImGui::SameLine();
			ImGui::TextDisabled("(%d objects)", count);

			// Assign + Delete buttons
			if (selectedObjectId != -1) {
				ImGui::SameLine();
				if (ImGui::Button("Assign selected")) {
					scene.AssignObjectToLayer(selectedObjectId, layerName);
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Delete")) {
				// Optional: don't allow deleting base layer "1"
				if (layerName != "1") {
					// Move all objects on this layer back to layer 1
					for (int objID:layer->GetObjects()) {
						scene.AssignObjectToLayer(objID, "1");
					}

					scene.RemoveLayer(layerName);
				}
			}

			ImGui::PopID();
		}
	}
#endif // _DEBUG
}

// Public ImGui Level Panel Implementation
namespace LEPANELLEVEL {
#ifdef _DEBUG
	void DrawLevelPanel(LevelEditor& editor, Scene& scene,
						int& selectedIndex, int& selectedObjectId) {
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Level###LE_Level")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Level Management");

		// Level path row
		static char levelPathBuf[256] = "../levels/kitchen01.json";
		if (editor.levelPath.empty()) {
			editor.levelPath = levelPathBuf;
		}

		static std::vector<std::string> sLevelFiles = ListJsonFiles("../levels");

		ImGui::TextUnformatted("Level path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##levels")) {
			sLevelFiles = ListJsonFiles("../levels");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##LevelCombo", editor.levelPath.c_str())) {
			for (size_t i = 0; i < sLevelFiles.size(); ++i) {
				const bool isSelected = (sLevelFiles[i] == editor.levelPath);
				if (ImGui::Selectable(sLevelFiles[i].c_str(), isSelected)) {
					editor.levelPath = sLevelFiles[i];
					std::snprintf(levelPathBuf, sizeof(levelPathBuf), "%s", editor.levelPath.c_str());
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputText("##LevelPathEdit", levelPathBuf, IM_ARRAYSIZE(levelPathBuf))) {
			editor.levelPath = levelPathBuf;
		}

		// Load
		if (ImGui::Button("Load Level")) {
			LevelData& work = editor.MutableLevel();
			if (LevelSerializer::Load(editor.levelPath, work)) {
				scene.ClearAll();
				SyncLevelToScene(work, scene);
				scene.RebuildColliders();
				scene.SetSimulationActive(false);
				scene.ResetResizeBaseline();

				if (!work.background.empty()) {
					scene.SetSceneBackground(work.background);
				}

				editor.SetPlaying(false);
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		ImGui::SameLine();

		// Save
		if (ImGui::Button("Save Level")) {
			LevelData& dst = editor.MutableLevel();
			SyncSceneToLevel(scene, dst);
			LevelSerializer::Save(editor.levelPath, dst);
		}

		ImGui::SameLine();

		// Undo
		if (ImGui::Button("Undo")) {
			if (PerformUndo(editor, scene)) {
				// visually reset selection
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		// Ctrl+Z keyboard shortcut for Undo (same as button)
		ImGuiIO& io = ImGui::GetIO();
		if (!editor.IsPlaying() &&
			!io.WantCaptureKeyboard &&
			(io.KeyCtrl || io.KeySuper) &&
			ImGui::IsKeyPressed(ImGuiKey_Z)) {
			if (PerformUndo(editor, scene)) {
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		ImGui::SameLine();

		// Play
		if (ImGui::Button(editor.IsPlaying()?"Playing...":"Play")) {
			if (!editor.IsPlaying()) {
				LevelData& snap = editor.MutablePlaySnapshot();
				SyncSceneToLevel(scene, snap);

				editor.SetPlaying(true);
				selectedIndex = -1;
				selectedObjectId = -1;

				scene.SetSimulationActive(true);
				scene.ClearAll();
				SyncLevelToScene(snap, scene);
				scene.RebuildColliders();
				scene.ResolveInitialStaticOverlaps();
			}
		}

		ImGui::SameLine();

		// Stop
		if (ImGui::Button("Stop")) {
			if (editor.IsPlaying()) {
				scene.ClearAll();
				SyncLevelToScene(editor.MutablePlaySnapshot(), scene);
				scene.RebuildColliders();
				scene.SetSimulationActive(false);
				editor.SetPlaying(false);
			}
		}

		DrawLayerManager(scene, selectedObjectId);

		ImGui::SeparatorText("Hierarchy");

		// Object Hierarchy – stable order independent of movement
		std::vector<GameObject*> objectList = scene.GetAllObjectsRaw();

		// Remove objects whose layer is currently hidden
		objectList.erase(
			std::remove_if(objectList.begin(), objectList.end(),
						   [&](GameObject* g) {
			if (!g) {
				return true;
			}
			std::string layerName = scene.GetObjectLayer(g->GetID());
			Layer* layer = scene.GetLayer(layerName);
			return (layer && !layer->IsVisible());
		}),
			objectList.end());

		// Sort by ID so list doesn’t reshuffle when objects move
		std::sort(objectList.begin(), objectList.end(),
				  [](GameObject* a, GameObject* b) {
			return a->GetID() < b->GetID();
		});

		// Keep hierarchy row in sync with selection by ID (click in Scene)
		if (selectedObjectId != -1) {
			int foundIndex = -1;
			for (int i = 0; i < static_cast<int>(objectList.size()); ++i) {
				GameObject* g = objectList[i];
				if (g && g->GetID() == selectedObjectId) {
					foundIndex = i;
					break;
				}
			}

			selectedIndex = foundIndex;

			// If the object was deleted or is on a hidden layer, clear selection
			if (selectedIndex == -1) {
				selectedObjectId = -1;
			}
		}

		// Hierarchy
		if (ImGui::BeginListBox("Objects", ImVec2(-FLT_MIN, 200.0f))) {
			for (int i = 0; i < static_cast<int>(objectList.size()); ++i) {
				GameObject* g = objectList[i];
				if (!g) {
					continue;
				}

				const int gid = g->GetID();
				std::string niceName;

				// Prefer tag; fall back to texture stem
				Scene::Defaults defs = scene.GetDefaults(gid);
				if (!defs.tag.empty()) {
					niceName = defs.tag;
				}
				else {
					std::string texPath = scene.GetObjectTexturePath(gid);
					if (!texPath.empty()) {
						try {
							niceName = fs::path(texPath).stem().string();
						}
						catch (...) {
						}
					}
				}

				std::string layer = scene.GetObjectLayer(gid);
				std::string label = niceName.empty()
					?("ID " + std::to_string(gid) + " [Layer: " + layer + "]")
					:(niceName + " (ID " + std::to_string(gid) + ") [Layer: " + layer + "]");

				ImGui::PushID(gid);
				bool isSelected = (selectedObjectId == gid);

				// When playing, draw items but DO NOT allow selection to change
				if (editor.IsPlaying()) {
					ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_Disabled);
				}
				else {
					if (ImGui::Selectable(label.c_str(), isSelected)) {
						selectedIndex = i;
						selectedObjectId = gid;
					}
				}

				ImGui::PopID();
			}

			ImGui::EndListBox();
		}

		if (editor.IsPlaying()) {
			ImGui::BeginDisabled();
		}

		// Add
		if (ImGui::Button("Add Object")) {
			// Snapshot BEFORE adding
			PushUndoSnapshot(editor, scene);

			LevelObject proto{};
			proto.texture = "../assets/goat_sprite_front.png";
			proto.tag = "npc";
			proto.x = 300.f; proto.y = 300.f; proto.z = 0.f;
			proto.w = 128.f; proto.h = 128.f;
			proto.layer = "1";
			proto.rotation = 0.f;

			if (GameObject* obj = scene.SpawnStaticSprite(proto.texture, { proto.x, proto.y, proto.z }, { proto.w, proto.h }, proto.layer)) {
				obj->SetColliderSize({ proto.colWidth, proto.colHeight });
				obj->SetColliderOffset({ proto.colOffsetX, proto.colOffsetY });

				scene.SetObjectTexturePath(obj->GetID(), proto.texture);
				scene.SetTransformFromLevel(obj->GetID(), { proto.x, proto.y, proto.z }, { proto.w, proto.h, 1.0f }, proto.rotation);

				Scene::Defaults defs{};
				defs.pos = { proto.x, proto.y, proto.z };
				defs.size = { proto.w, proto.h };
				defs.rot = proto.rotation;
				defs.colSize = { proto.colWidth, proto.colHeight };
				defs.colOff = { proto.colOffsetX, proto.colOffsetY };
				defs.vel = { proto.speedX, proto.speedY };
				defs.texture = proto.texture;
				defs.tag = proto.tag;
				defs.layer = proto.layer;
				scene.SetDefaults(obj->GetID(), defs);

				scene.ClampToWalkArea(obj);
			}
		}

		ImGui::SameLine();

		// Remove
		if (ImGui::Button("Remove Selected") &&
			selectedIndex >= 0 && selectedIndex < static_cast<int>(objectList.size()) && objectList[selectedIndex]) {
			// Snapshot BEFORE removing
			PushUndoSnapshot(editor, scene);

			scene.DespawnByID(objectList[selectedIndex]->GetID());
			selectedIndex = -1;
			selectedObjectId = -1;
		}

		if (editor.IsPlaying()) {
			ImGui::EndDisabled();
		}

		// Inspector
		if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objectList.size()) && objectList[selectedIndex]) {
			if (editor.IsPlaying()) {
				ImGui::BeginDisabled();
			}

			GameObject* obj = objectList[selectedIndex];
			const int id = obj->GetID();

			ImGui::SeparatorText("Properties Inspector");
			ImGui::TextDisabled("Selected ID: %d", id);

			// Gather current values
			char textureBuf[256];
			{
				std::string texPath = scene.GetObjectTexturePath(id);
				if (texPath.empty()) texPath = "../assets/goat_sprite_front.png";
				std::snprintf(textureBuf, sizeof(textureBuf), "%s", texPath.c_str());
			}

			char tagBuf[64] = "";
			if (id == scene.GetPlayerID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "player");
			}
			else if (id == scene.GetNPC1ID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "npc1");
			}
			else if (id == scene.GetNPC2ID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "npc2");
			}
			else if (id == scene.GetDinoID()) {
				std::snprintf(tagBuf, sizeof(tagBuf), "dino");
			}

			glm::vec3 position = obj->GetPositionGLM();
			glm::vec3 size = obj->GetScaleGLM();

			float rotationDeg = glm::degrees(obj->GetRotationAngleZ());

			if (!std::isfinite(rotationDeg)) {
				rotationDeg = 0.0f;
				// Also push this clean value into the object so it doesn’t stay corrupted
				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });
			}

			// Normalize inspector angle so it never shows crazy values
			rotationDeg = std::fmod(rotationDeg, 360.0f);
			if (rotationDeg < 0.0f) {
				rotationDeg += 360.0f;
			}

			auto colliderSize = obj->GetColliderSize();
			auto colliderOff = obj->GetColliderOffset();
			glm::vec2 velocity = scene.GetNPCVelocity(id);

			const auto defaults = scene.GetDefaults(id);

			// Helpers with right-click reset
			auto DragVec2WithReset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
				bool changed = ImGui::DragFloat2(label, v, speed);

				// First frame user starts dragging this control to capture pre-edit state
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}

				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						PushUndoSnapshot(editor, scene); // snapshot before reset
						v[0] = d.x;
						v[1] = d.y;
						apply(true);
					}

					ImGui::EndPopup();
				}

				if (changed) {
					apply(false);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Right-click to reset");
				}
			};

			auto DragFloatWithReset = [&](const char* label, float* v, float d, float speed, auto apply) {
				bool changed = ImGui::DragFloat(label, v, speed);

				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}

				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						PushUndoSnapshot(editor, scene); // snapshot before reset
						*v = d;
						apply(true);
					}

					ImGui::EndPopup();
				}

				if (changed) {
					apply(false);
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Right-click to reset");
				}
			};

			ImGui::Columns(2, nullptr, false);
			ImGuiStyle& style = ImGui::GetStyle();

			float longestLabel = 0.0f;
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Texture").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Tag").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Position (x,y)").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Size (w,h)").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Rotation (deg)").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Collider (w,h)").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Collider offset").x);
			longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Velocity (x,y)").x);
			float labelColWidth = longestLabel + style.ItemInnerSpacing.x * 2.0f + 12.0f;
			ImGui::SetColumnWidth(0, labelColWidth);

			auto FullWidthNext = []() {
				ImGui::SetNextItemWidth(-FLT_MIN);
			};

			// Texture
			ImGui::Text("Texture"); ImGui::NextColumn();
			FullWidthNext();
			bool texEdited = ImGui::InputText("##TexturePath", textureBuf, IM_ARRAYSIZE(textureBuf));

			// When user first clicks into the texture field, snapshot current state
			if (ImGui::IsItemActivated()) {
				PushUndoSnapshot(editor, scene);
			}

			if (texEdited) {
				std::string newPath(textureBuf);
				scene.SetObjectTexturePath(id, newPath);
				if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + newPath), newPath)) {
					obj->SetTexture(tex);
					if (newPath.find("dino_") != std::string::npos) {
						scene.AttachDinoAnimations(id);
						scene.SetAnimation(id, "IDLE");
						scene.MarkAnimated(id, true);
					}
					else {
						obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
						scene.MarkAnimated(id, false);
					}
				}
			}

			ImGui::NextColumn();

			// Tag
			ImGui::Text("Tag"); ImGui::NextColumn();
			FullWidthNext();
			ImGui::InputText("##Tag", tagBuf, IM_ARRAYSIZE(tagBuf));

			// Snapshot when user starts editing the tag
			if (ImGui::IsItemActivated()) {
				PushUndoSnapshot(editor, scene);
			}

			ImGui::NextColumn();

			// Layer
			ImGui::Text("Layer"); ImGui::NextColumn();
			FullWidthNext();

			// Current layer name (fallback to "Default" if empty)
			std::string currentLayer = scene.GetObjectLayer(id);
			if (currentLayer.empty()) {
				currentLayer = "1";
			}

			// Build a sorted list of layer names (always include "Default")
			std::vector<std::string> layerNames;
			layerNames.reserve(scene.GetAllLayers().size() + 1);
			layerNames.push_back("1");

			const auto& allLayers = scene.GetAllLayers();
			for (const auto& pair : allLayers) {
				const std::string& name = pair.first;
				if (name.empty()) {
					continue;
				}
				if (std::find(layerNames.begin(), layerNames.end(), name) == layerNames.end()) {
					layerNames.push_back(name);
				}
			}

			std::sort(layerNames.begin(), layerNames.end());

			const char* previewLayer = currentLayer.c_str();
			if (ImGui::BeginCombo("##Layer", previewLayer)) {
				for (const std::string& name : layerNames) {
					bool isSelected = (currentLayer == name);
					if (ImGui::Selectable(name.c_str(), isSelected)) {
						// Snapshot before changing the layer
						PushUndoSnapshot(editor, scene);
						scene.AssignObjectToLayer(id, name);
						currentLayer = name;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::NextColumn();

			// Position
			ImGui::Text("Position (x,y)"); ImGui::NextColumn();
			FullWidthNext();
			DragVec2WithReset("##pos", &position.x, ImVec2(defaults.pos.x, defaults.pos.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				scene.ClampToWalkArea(obj);
			});
			ImGui::NextColumn();

			// Size
			ImGui::Text("Size (w,h)"); ImGui::NextColumn();
			FullWidthNext();
			DragVec2WithReset("##size", &size.x, ImVec2(defaults.size.x, defaults.size.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				scene.ClampToWalkArea(obj);
			});
			ImGui::NextColumn();

			// Animation
			ImGui::Separator();
			ImGui::Text("Animation");
			ImGui::NextColumn();
			FullWidthNext();

			if (id != -1) {
				bool hasAnimator = scene.HasAnimations(id); // auto-detected from scene
				ImGui::BeginDisabled();                     // make the checkbox read-only
				ImGui::Checkbox("Animated", &hasAnimator);
				ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::TextDisabled("(auto-detected)");

				if (hasAnimator) {
					std::vector<std::string> animList = scene.GetAnimationList(id);
					std::string current = scene.GetCurrentAnimationName(id);

					if (animList.empty()) {
						ImGui::TextDisabled("No clips found");
					}
					else {
						const char* preview = current.c_str();
						if (ImGui::BeginCombo("Current", preview)) {
							for (const std::string& name : animList) {
								bool selected = (current == name);

								if (ImGui::Selectable(name.c_str(), selected)) {
									scene.SetAnimation(id, name.c_str());
								}

								if (selected) {
									ImGui::SetItemDefaultFocus();
								}
							}

							ImGui::EndCombo();
						}
					}
				}
			}

			ImGui::NextColumn();

			// Rotation
			ImGui::Text("Rotation (deg)"); ImGui::NextColumn();
			FullWidthNext();
			DragFloatWithReset("##rot", &rotationDeg, defaults.rot, 0.05f, [&](bool) {
				// Clamp to [0, 360) before applying so it never stores huge angles
				rotationDeg = std::fmod(rotationDeg, 360.0f);
				if (rotationDeg < 0.0f) {
					rotationDeg += 360.0f;
				}

				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
			});
			ImGui::NextColumn();

			// Collider size
			ImGui::Text("Collider (w,h)"); ImGui::NextColumn();
			FullWidthNext();
			DragVec2WithReset("##colsz", &colliderSize.x, ImVec2(defaults.colSize.x, defaults.colSize.y), 1.0f, [&](bool) {
				obj->SetColliderSize({ colliderSize.x, colliderSize.y });
				scene.RebuildColliders();
			});
			ImGui::NextColumn();

			// Collider offset
			ImGui::Text("Collider offset"); ImGui::NextColumn();
			FullWidthNext();
			DragVec2WithReset("##coloff", &colliderOff.x, ImVec2(defaults.colOff.x, defaults.colOff.y), 1.0f, [&](bool) {
				obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
				scene.RebuildColliders();
			});
			ImGui::NextColumn();

			// Velocity
			ImGui::Text("Velocity (x,y)"); ImGui::NextColumn();
			FullWidthNext();
			DragVec2WithReset("##vel", &velocity.x, ImVec2(defaults.vel.x, defaults.vel.y), 1.0f, [&](bool) {
				scene.SetNPCVelocity(id, velocity.x, velocity.y);
			});
			ImGui::NextColumn();

			ImGui::Columns(1);

			// Apply updated values
			// scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
			obj->SetColliderSize({ colliderSize.x, colliderSize.y });
			obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
			scene.SetNPCVelocity(id, velocity.x, velocity.y);
			scene.RebuildColliders();

			// Handle special IDs if tag changed
			std::string newTag = tagBuf;
			if (newTag == "player") {
				scene.SetPlayerID(id);
			}
			else if (newTag == "npc1") {
				scene.SetNPC1ID(id);
			}
			else if (newTag == "npc2") {
				scene.SetNPC2ID(id);
			}
			else if (newTag == "dino") {
				scene.SetDinoID(id);
			}

			if (editor.IsPlaying()) {
				ImGui::EndDisabled();
			}
		}

		// Scene viewport area: DROP-ZONE ONLY (picking/dragging happens on the Scene tab)
		ImGui::Spacing();
		ImGui::TextDisabled("Drag & drop prefabs or textures here.");

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		if (viewportSize.y < 64.f) {
			viewportSize.y = 64.f;
		}

		if (viewportSize.x < 64.f) {
			viewportSize.x = 64.f;
		}

		// Passive area: still accepts drops
		ImGui::InvisibleButton("##SceneViewport", viewportSize, ImGuiButtonFlags_None);

		const bool viewportHovered = ImGui::IsItemHovered();
		const bool viewportActive = ImGui::IsItemActive();
		InputManager::Get().SetSceneViewportWantsGameMouse(viewportHovered || viewportActive);

		// Allow dropping prefabs/textures here
		std::vector<GameObject*> objectListForViewport;
		scene.CollectRenderablePointers(objectListForViewport);

		// NOTE: use editor.IsPlaying() (not isPlaying) and LEIO/LELINKS namespaces
		if (!editor.IsPlaying() && ImGui::BeginDragDropTarget()) {
			// Prefab dropped to instantiate
			if (const ImGuiPayload* pp = ImGui::AcceptDragDropPayload("PREFAB_PATH")) {
				const char* droppedCStr = static_cast<const char*>(pp->Data);
				const std::string dropped = droppedCStr?std::string(droppedCStr):std::string();

				LevelObject data{};
				if (LEFILEIO::LoadPrefabFromFile(dropped, data)) {
					// Snapshot BEFORE creating instance from prefab
					PushUndoSnapshot(editor, scene);

					std::string prefabLayer = data.layer.empty()?"1":data.layer;
					GameObject* g = scene.SpawnStaticSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						prefabLayer);

					if (g) {
						// link prefab to instance for propagation
						LELINKS::PrefabLinkByID[g->GetID()] = dropped;

						g->SetRotation(glm::radians(data.rotation), { 0, 0, 1 });
						g->SetColliderSize({ data.colWidth,  data.colHeight });
						g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });

						scene.SetObjectTexturePath(g->GetID(), data.texture);
						scene.SetTransformFromLevel(g->GetID(),
													{ data.x, data.y, data.z }, { data.w, data.h, 1.0f }, data.rotation);
						scene.SetNPCVelocity(g->GetID(), data.speedX, data.speedY);
						scene.ClampToWalkArea(g);
					}
				}
			}

			// Texture dropped to apply to selected
			if (const ImGuiPayload* tp = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
				const char* droppedCStr = static_cast<const char*>(tp->Data);
				const std::string dropped = droppedCStr?std::string(droppedCStr):std::string();

				if (selectedObjectId != -1) {
					// Find the selected object by ID in the render-sorted list
					GameObject* o = nullptr;
					for (GameObject* cand : objectListForViewport) {
						if (cand && cand->GetID() == selectedObjectId) {
							o = cand;
							break;
						}
					}

					if (o) {
						// Snapshot BEFORE applying new texture
						PushUndoSnapshot(editor, scene);

						// GameObject* o = objectListForViewport[selectedIndex];
						const int id2 = o->GetID();

						// Store path in scene metadata
						scene.SetObjectTexturePath(id2, dropped);
						if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + dropped), dropped)) {
							o->SetTexture(tex);

							// If this is one of your animated dino sprites, wire up animation
							if (dropped.find("dino_") != std::string::npos) {
								scene.AttachDinoAnimations(id2);
								scene.SetAnimation(id2, "IDLE");
								scene.MarkAnimated(id2, true);
							}
							else {
								// Non-animated: reset to full-frame UV and mark as static
								o->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
								scene.MarkAnimated(id2, false);
							}
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void RecordUndoSnapshot(LevelEditor& editor, Scene& scene) {
		PushUndoSnapshot(editor, scene);
	}
#else
	// Release: no ImGui, provide no-op implementations so callers still link.
	void DrawLevelPanel(LevelEditor& /*editor*/, Scene& /*scene*/, int& /*selectedIndex*/, int& /*selectedObjectId*/) {
		// Editor UI disabled in Release builds.
	}

	void RecordUndoSnapshot(LevelEditor& /*editor*/, Scene& /*scene*/) {
		// No-op in Release.
	}
#endif
}

