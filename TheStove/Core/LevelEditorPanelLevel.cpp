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

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorPanelLevel.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "LevelEditorPrefabLinks.hpp"

#include "InputManager.hpp"

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fs = std::filesystem;

using namespace LEFILEIO;

namespace {
	// Internal helpers for Level <-> Scene synchronization

	// Build the current scene from loaded LevelData.
	void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
		for (const auto& obj : levelIn.objects) {
			GameObject* g = nullptr;

			// Spawn animated or static
			if (obj.animated) {
				const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
				g = scene.SpawnAnimatedSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h },
					fullFrame, 0.25f, true, obj.layer);

				if (obj.texture.find("dino") != std::string::npos) {
					scene.AttachDinoAnimations(g->GetID());
					scene.SetAnimation(g->GetID(), "IDLE");
				}
			}
			else {
				g = scene.SpawnStaticSprite(obj.texture, { obj.x, obj.y, 0.0f }, { obj.w, obj.h }, obj.layer);
			}

			if (!g) {
				std::cerr << "Spawn failed: " << obj.texture << std::endl;
				continue;
			}

			// Rotation (editor stores degrees; GameObject uses radians)
			g->SetRotation(glm::radians(obj.rotation), { 0, 0, 1 });

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

			scene.SetDefaults(g->GetID(), defs);
			scene.AttachLogicForTag(g->GetID(), obj.tag);
			scene.ClampToWalkArea(g);
		}
	}

	// Build LevelData snapshot from the current scene.
	void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
		levelOut.objects.clear();

		std::vector<GameObject*> list;
		scene.CollectRenderablePointers(list);

		for (GameObject* g : list) {
			if (!g) {
				continue;
			}

			LevelObject out{};
			out.texture = scene.GetObjectTexturePath(g->GetID());
			out.animated = scene.HasAnimations(g->GetID());
			out.layer = scene.GetObjectLayer(g->GetID());

			const glm::vec3 p = g->GetPositionGLM();
			const glm::vec3 s = g->GetScaleGLM();

			out.x = p.x; out.y = p.y; out.z = p.z;
			out.w = s.x; out.h = s.y;
			out.rotation = glm::degrees(g->GetRotationAngleZ());

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
				out.tag.clear();
			}

			const glm::vec2 v = scene.GetNPCVelocity(g->GetID());
			out.speedX = v.x; out.speedY = v.y;

			levelOut.objects.push_back(out);
		}
	}
}

// Public ImGui Level Panel Implementation
namespace LEPANELLEVEL {
	void DrawLevelPanel(LevelEditor& editor, Scene& scene,
		int& selectedIndex, int& selectedObjectId)
	{
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Level###LE_Level")) {
			ImGui::End();
			return;
		}

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

		// Play
		if (ImGui::Button(editor.IsPlaying() ? "Playing..." : "Play")) {
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

		ImGui::Separator();

		// Object Hierarchy
		std::vector<GameObject*> objectList;
		scene.CollectRenderablePointers(objectList);

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
						try { niceName = fs::path(texPath).stem().string(); }
						catch (...) {}
					}
				}

				std::string layer = scene.GetObjectLayer(gid);
				std::string label = niceName.empty()
					? ("ID " + std::to_string(gid) + " [Layer: " + layer + "]")
					: (niceName + " (ID " + std::to_string(gid) + ") [Layer: " + layer + "]");

				if (ImGui::Selectable(label.c_str(), selectedIndex == i)) {
					selectedIndex = i;
					selectedObjectId = gid;
				}
			}

			ImGui::EndListBox();
		}

		if (editor.IsPlaying()) {
			ImGui::BeginDisabled();
		}

		// Add
		if (ImGui::Button("Add Object")) {
			LevelObject proto{};
			proto.texture = "../assets/goat_sprite_front.png";
			proto.tag = "npc";
			proto.x = 300.f; proto.y = 300.f; proto.z = 0.f;
			proto.w = 128.f; proto.h = 128.f;
			proto.rotation = 0.f;

			if (GameObject* obj = scene.SpawnStaticSprite(proto.texture, { proto.x, proto.y, proto.z }, { proto.w, proto.h })) {
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
				scene.SetDefaults(obj->GetID(), defs);

				scene.ClampToWalkArea(obj);
			}
		}

		ImGui::SameLine();

		// Remove
		if (ImGui::Button("Remove Selected") &&
			selectedIndex >= 0 && selectedIndex < static_cast<int>(objectList.size()) && objectList[selectedIndex]) {
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

			ImGui::Separator();
			ImGui::Text("Properties (ID %d)", id);

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
			auto colliderSize = obj->GetColliderSize();
			auto colliderOff = obj->GetColliderOffset();
			glm::vec2 velocity = scene.GetNPCVelocity(id);

			const auto defaults = scene.GetDefaults(id);

			// Helpers with right-click reset
			auto DragVec2WithReset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
				bool changed = ImGui::DragFloat2(label, v, speed);
				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) { v[0] = d.x; v[1] = d.y; apply(true); }
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
				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						*v = d; apply(true);
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
			if (ImGui::InputText("##TexturePath", textureBuf, IM_ARRAYSIZE(textureBuf))) {
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
			ImGui::NextColumn();

			// Layer
			ImGui::Text("Layer"); ImGui::NextColumn();
			FullWidthNext();

			char layerBuf[64] = "";
			std::string layerName = scene.GetObjectLayer(id);
			std::snprintf(layerBuf, sizeof(layerBuf), "%s", layerName.c_str());

			ImGui::InputText("##Layer", layerBuf, IM_ARRAYSIZE(layerBuf), ImGuiInputTextFlags_ReadOnly);
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

			// Rotation
			ImGui::Text("Rotation (deg)"); ImGui::NextColumn();
			FullWidthNext();
			DragFloatWithReset("##rot", &rotationDeg, defaults.rot, 0.25f, [&](bool) {
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
			scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
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
		ImGui::Separator();
		ImGui::TextDisabled("Drop prefab to instantiate,\nor texture to apply to selected");

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
				const std::string dropped = droppedCStr ? std::string(droppedCStr) : std::string();

				LevelObject data{};
				if (LEFILEIO::LoadPrefabFromFile(dropped, data)) {
					GameObject* g = scene.SpawnStaticSprite(
						data.texture, { data.x, data.y, data.z }, { data.w, data.h });

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
				const std::string dropped = droppedCStr ? std::string(droppedCStr) : std::string();

				if (!objectListForViewport.empty() &&
					selectedIndex >= 0 &&
					selectedIndex < static_cast<int>(objectListForViewport.size()) &&
					objectListForViewport[selectedIndex]) {
					GameObject* o = objectListForViewport[selectedIndex];
					const int id2 = o->GetID();

					scene.SetObjectTexturePath(id2, dropped);
					if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + dropped), dropped)) {
						o->SetTexture(tex);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

}
