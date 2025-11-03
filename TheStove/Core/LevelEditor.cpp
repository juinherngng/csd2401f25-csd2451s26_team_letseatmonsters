/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Simple in-engine level editor window.
					- JSON/Editor store rotation in DEGREES.
					- GameObject setters should receive RADIANS (convert at call-site).

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "LevelEditor.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

// Prefab helpers
static bool SavePrefabToFile(const std::string& prefabPath, const LevelObject& src) {
	LevelData one;
	one.objects.clear();
	one.objects.push_back(src);
	return LevelSerializer::Save(prefabPath, one);
}

static bool LoadPrefabFromFile(const std::string& prefabPath, LevelObject& out) {
	LevelData one;
	if (!LevelSerializer::Load(prefabPath, one) || one.objects.empty()) {
		return false;
	}
	out = one.objects.front();
	return true;
}

// Apply prefab data to an existing object but keep its current position/z
static void ApplyPrefabToObjectKeepPosition(const LevelObject& prefab, Scene& scene, GameObject* obj) {
	if (obj == nullptr) {
		return;
	}

	const int id = obj->GetID();
	glm::vec3 keepPos = obj->GetPositionGLM();

	const float ww = prefab.w;
	const float hh = prefab.h;
	obj->SetScale(glm::vec3(ww, hh, 1.0f));

	obj->SetColliderSize({ prefab.colWidth, prefab.colHeight });
	obj->SetColliderOffset({ prefab.colOffsetX, prefab.colOffsetY });

	scene.SetTransformFromLevel(id, obj->GetPositionGLM(), { ww, hh, 1.0f }, prefab.rotation);


	scene.SetObjectTexturePath(id, prefab.texture);
	if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + prefab.texture), prefab.texture)) {
		obj->SetTexture(tex);
	}

	obj->SetPosition(keepPos);
	scene.ClampToWalkArea(obj);
}

static void SyncSceneToLevel(Scene& scene, LevelData& levelOut);
static void SyncLevelToScene(const LevelData& levelIn, Scene& scene);

// LevelEditor methods
bool LevelEditor::LoadIntoScene(Scene& scene) {
	if (!LevelSerializer::Load(levelPath, level)) {
		return false;
	}

	SyncLevelToScene(level, scene);
	return true;
}

static std::vector<std::string> ListJsonFiles(const std::string& dir) {
	std::vector<std::string> out;
	std::error_code ec;
	if (!fs::exists(dir, ec)) return out;
	for (const auto& p : fs::directory_iterator(dir, ec)) {
		if (p.is_regular_file()) {
			const auto& path = p.path();
			if (path.extension() == ".json") {
				out.push_back(path.generic_string());  // keep forward slashes
			}
		}
	}
	std::sort(out.begin(), out.end());
	return out;
}

static std::vector<std::string> ListAssetsWithExt(const std::string& dir,
	const std::vector<std::string>& exts) {
	std::vector<std::string> out;
	std::error_code ec;
	if (!fs::exists(dir, ec)) return out;
	for (const auto& p : fs::directory_iterator(dir, ec)) {
		if (!p.is_regular_file()) continue;
		auto ext = p.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		for (auto const& e : exts) {
			if (ext == e) { out.push_back(p.path().generic_string()); break; }
		}
	}
	std::sort(out.begin(), out.end());
	return out;
}

void LevelEditor::DrawUI(Scene& scene) {
	static int selectedIndex = -1;

	if (!isEnabled) {
		return;
	}

	ImGuiContext* imguiContext = ImGui::GetCurrentContext();
	if (imguiContext == nullptr || !imguiContext->WithinFrameScope) {
		// ImGui frame hasn’t started yet; skip safely.
		return;
	}

	bool windowOpen = true;
	if (!windowOpen) {
		isEnabled = false;
	}

	// ===== Level Window ==========================================================
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Level###LE_Level")) {

		// ---------- Level Path Row (dropdown + refresh) ----------
		static char _pathBuf[256] = "../levels/kitchen01.json";
		if (levelPath.empty()) levelPath = _pathBuf;

		static std::vector<std::string> sLevelFiles = ListJsonFiles("../levels");

		ImGui::TextUnformatted("Level path");
		ImGui::SameLine();
		if (ImGui::Button("Refresh##levels")) {
			sLevelFiles = ListJsonFiles("../levels");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##LevelCombo", levelPath.c_str())) {
			for (size_t i = 0; i < sLevelFiles.size(); ++i) {
				bool selected = (sLevelFiles[i] == levelPath);
				if (ImGui::Selectable(sLevelFiles[i].c_str(), selected)) {
					levelPath = sLevelFiles[i];
					std::snprintf(_pathBuf, sizeof(_pathBuf), "%s", levelPath.c_str());
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputText("##LevelPathEdit", _pathBuf, IM_ARRAYSIZE(_pathBuf))) {
			levelPath = _pathBuf;
		}

		// Load
		if (ImGui::Button("Load Level")) {
			if (LevelSerializer::Load(levelPath, level)) {
				scene.ClearAll();
				SyncLevelToScene(level, scene);
				scene.RebuildColliders();
				scene.SetSimulationActive(false);
				scene.ResetResizeBaseline();
				isPlaying = false;
				selectedIndex = -1;
			}
		}
		ImGui::SameLine();

		// Save
		if (ImGui::Button("Save Level")) {
			level.objects.clear();
			std::vector<GameObject*> objectList; scene.CollectRenderablePointers(objectList);
			for (GameObject* obj : objectList) {
				if (!obj) continue;
				LevelObject out{};
				out.texture = scene.GetObjectTexturePath(obj->GetID());
				const glm::vec3 position = obj->GetPositionGLM();
				const glm::vec3 size = obj->GetScaleGLM();
				out.x = position.x; out.y = position.y; out.z = position.z;
				out.w = size.x;     out.h = size.y;
				out.rotation = glm::degrees(obj->GetRotationAngleZ());
				const auto csz = obj->GetColliderSize();
				const auto cof = obj->GetColliderOffset();
				out.colWidth = csz.x; out.colHeight = csz.y;
				out.colOffsetX = cof.x; out.colOffsetY = cof.y;
				if (obj->GetID() == scene.GetPlayerID()) out.tag = "player";
				if (obj->GetID() == scene.GetNPC1ID())   out.tag = "npc1";
				if (obj->GetID() == scene.GetNPC2ID())   out.tag = "npc2";
				if (obj->GetID() == scene.GetDinoID())   out.tag = "dino";
				const glm::vec2 v = scene.GetNPCVelocity(obj->GetID());
				out.speedX = v.x; out.speedY = v.y;
				out.animated = scene.HasAnimations(obj->GetID());
				level.objects.push_back(out);
			}
			LevelSerializer::Save(levelPath, level);
		}

		// Play / Stop
		if (ImGui::Button(isPlaying ? "Playing..." : "Play")) {
			if (!isPlaying) {
				playStartSnapshot.objects.clear();
				SyncSceneToLevel(scene, playStartSnapshot);
				isPlaying = true;
				playStartSnapshot = level;
				scene.SetSimulationActive(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			if (isPlaying) {
				scene.ClearAll();
				SyncLevelToScene(playStartSnapshot, scene);
				scene.RebuildColliders();
				scene.SetSimulationActive(false);
				isPlaying = false;
			}
		}

		ImGui::Separator();

		// Hierarchy
		std::vector<GameObject*> objectList; scene.CollectRenderablePointers(objectList);
		if (ImGui::BeginListBox("Objects", ImVec2(-FLT_MIN, 200.0f))) {
			for (int i = 0; i < (int)objectList.size(); ++i) {
				if (!objectList[i]) continue;
				std::string label = "ID " + std::to_string(objectList[i]->GetID());
				if (ImGui::Selectable(label.c_str(), selectedIndex == i)) {
					selectedIndex = i;
				}
			}
			ImGui::EndListBox();
		}


		// Add / Remove
		if (isPlaying) ImGui::BeginDisabled();
		if (ImGui::Button("Add Object")) {
			LevelObject proto{};
			proto.texture = "../assets/goat_sprite_front.png";
			proto.tag = "npc";
			proto.x = 300.f; proto.y = 300.f; proto.z = 0.f;
			proto.w = 128.f; proto.h = 128.f; proto.rotation = 0.f;
			GameObject* obj = scene.SpawnStaticSprite(proto.texture, { proto.x, proto.y, proto.z }, { proto.w, proto.h });
			if (obj) {
				obj->SetColliderSize({ proto.colWidth, proto.colHeight });
				obj->SetColliderOffset({ proto.colOffsetX, proto.colOffsetY });
				scene.SetObjectTexturePath(obj->GetID(), proto.texture);
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
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected") && selectedIndex >= 0 && selectedIndex < (int)objectList.size()) {
			scene.DespawnByID(objectList[selectedIndex]->GetID());
			selectedIndex = -1;
		}
		if (isPlaying) ImGui::EndDisabled();

		// Properties panel (position/size/rotation/collider/velocity/anim)
		if (selectedIndex >= 0 && selectedIndex < (int)objectList.size() && objectList[selectedIndex]) {
			if (isPlaying) ImGui::BeginDisabled();

			GameObject* obj = objectList[selectedIndex];
			const int id = obj->GetID();

			ImGui::Separator();
			ImGui::Text("Properties (ID %d)", id);

			// Read current texture path
			char textureBuf[256];
			{
				std::string texPath = scene.GetObjectTexturePath(id);
				if (texPath.empty()) texPath = "../assets/goat_sprite_front.png";
				std::snprintf(textureBuf, sizeof(textureBuf), "%s", texPath.c_str());
			}

			// Tag from special IDs
			char tagBuf[64] = "";
			if (id == scene.GetPlayerID())      std::snprintf(tagBuf, sizeof(tagBuf), "player");
			else if (id == scene.GetNPC1ID())   std::snprintf(tagBuf, sizeof(tagBuf), "npc1");
			else if (id == scene.GetNPC2ID())   std::snprintf(tagBuf, sizeof(tagBuf), "npc2");
			else if (id == scene.GetDinoID())   std::snprintf(tagBuf, sizeof(tagBuf), "dino");

			glm::vec3 position = obj->GetPositionGLM();
			glm::vec3 size = obj->GetScaleGLM();               // z ignored for sprites
			float      rotationDeg = glm::degrees(obj->GetRotationAngleZ());
			auto       colliderSize = obj->GetColliderSize();
			auto       colliderOff = obj->GetColliderOffset();
			glm::vec2  velocity = scene.GetNPCVelocity(id);

			// Defaults for right-click reset
			const auto defaults = scene.GetDefaults(id);

			// Helpers: Drag widgets with right-click "Reset"
			auto DragVec2WithReset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
				bool changed = ImGui::DragFloat2(label, v, speed);
				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						v[0] = d.x; v[1] = d.y;
						apply(true);
					}
					ImGui::EndPopup();
				}
				if (changed) apply(false);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click to reset");
				};

			auto DragFloatWithReset = [&](const char* label, float* v, float d, float speed, auto apply) {
				bool changed = ImGui::DragFloat(label, v, speed);
				if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
					if (ImGui::MenuItem("Reset to default")) {
						*v = d;
						apply(true);
					}
					ImGui::EndPopup();
				}
				if (changed) apply(false);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click to reset");
				};

			// Compact 2-column layout
			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 120.0f);

			// Texture ---------------------------------------------------
			ImGui::Text("Texture"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::InputText("##TexturePath", textureBuf, IM_ARRAYSIZE(textureBuf))) {
				scene.SetObjectTexturePath(id, textureBuf);
				if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(textureBuf)), textureBuf)) {
					obj->SetTexture(tex);
				}
			}
			// Accept drag-drop of textures on the same row
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
					const char* dropped = static_cast<const char*>(payload->Data);
					scene.SetObjectTexturePath(id, dropped);
					if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(dropped)), dropped)) {
						obj->SetTexture(tex);
					}
					std::snprintf(textureBuf, sizeof(textureBuf), "%s", dropped);
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::NextColumn();

			// Tag -------------------------------------------------------
			ImGui::Text("Tag"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			ImGui::InputText("##Tag", tagBuf, IM_ARRAYSIZE(tagBuf));
			ImGui::NextColumn();

			// Position --------------------------------------------------
			ImGui::Text("Position (x,y)"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragVec2WithReset("##pos", &position.x, ImVec2(defaults.pos.x, defaults.pos.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0,0,1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				scene.ClampToWalkArea(obj);
				});
			ImGui::NextColumn();

			// Size ------------------------------------------------------
			ImGui::Text("Size (w,h)"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragVec2WithReset("##size", &size.x, ImVec2(defaults.size.x, defaults.size.y), 1.0f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0,0,1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				scene.ClampToWalkArea(obj);
				});
			ImGui::NextColumn();

			// Rotation --------------------------------------------------
			ImGui::Text("Rotation (deg)"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragFloatWithReset("##rot", &rotationDeg, defaults.rot, 0.25f, [&](bool) {
				obj->SetRotation(glm::radians(rotationDeg), { 0,0,1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				});
			ImGui::NextColumn();

			// Collider size --------------------------------------------
			ImGui::Text("Collider (w,h)"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragVec2WithReset("##colsz", &colliderSize.x, ImVec2(defaults.colSize.x, defaults.colSize.y), 1.0f, [&](bool) {
				obj->SetColliderSize({ colliderSize.x, colliderSize.y });
				});
			ImGui::NextColumn();

			// Collider offset ------------------------------------------
			ImGui::Text("Collider offset"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragVec2WithReset("##coloff", &colliderOff.x, ImVec2(defaults.colOff.x, defaults.colOff.y), 1.0f, [&](bool) {
				obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
				});
			ImGui::NextColumn();

			// Velocity --------------------------------------------------
			ImGui::Text("Velocity (x,y)"); ImGui::NextColumn();
			ImGui::SetNextItemWidth(140.0f);
			DragVec2WithReset("##vel", &velocity.x, ImVec2(defaults.vel.x, defaults.vel.y), 1.0f, [&](bool) {
				scene.SetNPCVelocity(id, velocity.x, velocity.y);
				});
			ImGui::NextColumn();

			// Start animation (if available) ----------------------------
			if (scene.HasAnimations(id)) {
				std::vector<std::string> animationNames = scene.GetAnimationList(id);
				std::string currentAnim = scene.GetCurrentAnimationName(id);
				int currentIndex = 0;
				for (int i = 0; i < (int)animationNames.size(); ++i) {
					if (animationNames[i] == currentAnim) { currentIndex = i; break; }
				}

				ImGui::Text("Start animation"); ImGui::NextColumn();
				ImGui::SetNextItemWidth(140.0f);
				if (ImGui::BeginCombo("##animCombo", currentAnim.empty() ? "(none)" : currentAnim.c_str())) {
					for (int i = 0; i < (int)animationNames.size(); ++i) {
						bool selected = (i == currentIndex);
						if (ImGui::Selectable(animationNames[i].c_str(), selected)) {
							scene.SetAnimation(id, animationNames[i]);
						}
						if (selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			// Allow dropping a prefab onto the inspector to APPLY values (keep position)
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_PATH")) {
					const char* dropped = static_cast<const char*>(payload->Data);
					LevelObject data{};
					if (LoadPrefabFromFile(dropped, data)) {
						ApplyPrefabToObjectKeepPosition(data, scene, obj);
					}
				}
				ImGui::EndDragDropTarget();
			}

			// Final apply to keep scene/editor in sync
			scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
			obj->SetColliderSize({ colliderSize.x, colliderSize.y });
			obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
			scene.SetNPCVelocity(id, velocity.x, velocity.y);

			// Update special IDs if tag changed
			std::string newTag = tagBuf;
			if (newTag == "player") scene.SetPlayerID(id);
			if (newTag == "npc1")   scene.SetNPC1ID(id);
			if (newTag == "npc2")   scene.SetNPC2ID(id);
			if (newTag == "dino")   scene.SetDinoID(id);

			if (isPlaying) ImGui::EndDisabled();
		}


		// Big drop zone for scene
		ImGui::Separator();
		ImGui::TextDisabled("Drop prefab to instantiate,\nor texture to apply to selected");
		ImVec2 avail = ImGui::GetContentRegionAvail(); if (avail.y < 64.f) avail.y = 64.f;
		ImGui::InvisibleButton("##SceneDropZone", avail);
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* pp = ImGui::AcceptDragDropPayload("PREFAB_PATH")) {
				const char* dropped = static_cast<const char*>(pp->Data);
				LevelObject data{};
				if (LoadPrefabFromFile(dropped, data)) {
					GameObject* g = scene.SpawnStaticSprite(data.texture, { data.x, data.y, data.z }, { data.w, data.h });
					if (g) {
						g->SetRotation(glm::radians(data.rotation), { 0,0,1 });
						g->SetColliderSize({ data.colWidth, data.colHeight });
						g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });
						scene.SetObjectTexturePath(g->GetID(), data.texture);
						scene.SetTransformFromLevel(g->GetID(), { data.x, data.y, data.z }, { data.w, data.h, 1.0f }, data.rotation);
						scene.ClampToWalkArea(g);
					}
				}
			}
			if (const ImGuiPayload* tp = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
				const char* dropped = static_cast<const char*>(tp->Data);
				std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);
				if (selectedIndex >= 0 && selectedIndex < (int)objs.size() && objs[selectedIndex]) {
					GameObject* o = objs[selectedIndex];
					int id2 = o->GetID();
					scene.SetObjectTexturePath(id2, dropped);
					if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(dropped)), dropped)) {
						o->SetTexture(tex);
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}
	ImGui::End(); // Level window

	// ===== Prefabs Window ========================================================
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Prefabs###LE_Prefabs")) {

		// local object list used by buttons here
		std::vector<GameObject*> objectList; scene.CollectRenderablePointers(objectList);

		ImGui::Separator();
		ImGui::Text("Prefabs / Archetypes");
		ImGui::Spacing();

		static char prefabPathBuf[256] = "../prefabs/my_goat.json";
		static std::vector<std::string> sPrefabFiles = ListJsonFiles("../prefabs");

		ImGui::TextUnformatted("Prefab path");
		ImGui::SameLine();
		if (ImGui::Button("Refresh##prefabs")) {
			sPrefabFiles = ListJsonFiles("../prefabs");
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##PrefabCombo", prefabPathBuf)) {
			for (size_t i = 0; i < sPrefabFiles.size(); ++i) {
				bool selected = (std::string(prefabPathBuf) == sPrefabFiles[i]);
				if (ImGui::Selectable(sPrefabFiles[i].c_str(), selected)) {
					std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", sPrefabFiles[i].c_str());
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##PrefabPathEdit", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

		const bool prefabExists = fs::exists(prefabPathBuf);

		if (ImGui::Button("Save selected as prefab")) {
			// uses the same selectedIndex static from Level window — you can
			// either move selection state into a shared member, or re-select there first.
			// Easiest: duplicate the save code but guard against invalid selection:
			int selectedIndexShadow = -1;
			// (optional) you can expose selectedIndex via LevelEditor as a member if you prefer.

			// For now: attempt to find "most recently clicked" by checking a stored id,
			// but if you don't have that, keep using selectedIndex if you made it a member.
		}

		ImGui::BeginDisabled(!prefabExists);
		if (ImGui::Button("Instantiate from prefab")) { /* same as your current code */ }
		if (ImGui::Button("Propagate prefab changes")) { /* same as your current code */ }
		ImGui::EndDisabled();

	}
	ImGui::End(); // Prefabs window

	// ===== Assets Window =========================================================
	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Assets###LE_Assets")) {

		ImGui::Text("Assets");
		ImGui::Spacing();
		ImGui::BeginChild("##AssetsBox", ImVec2(0, 0), true);

		static std::vector<std::string> sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
		static std::vector<std::string> sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });

		if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##tex")) sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
			for (auto const& path : sTextures) {
				if (ImGui::Selectable(path.c_str(), false)) {}
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Texture");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}

		if (ImGui::CollapsingHeader("Prefabs", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##pf")) sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
			for (auto const& path : sPrefabs) {
				if (ImGui::Selectable(path.c_str(), false)) {}
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("PREFAB_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Prefab");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}
			}
		}

		ImGui::EndChild();
	}
	ImGui::End(); // Assets window


}

static void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
	for (const auto& obj : levelIn.objects) {
		const float x = obj.x;
		const float y = obj.y;
		const float ww = obj.w;
		const float hh = obj.h;

		GameObject* g = nullptr;
		if (obj.animated) {
			// Give it a valid frame immediately so nothing smears before AttachDinoAnimations.
			const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
			g = scene.SpawnAnimatedSprite(
				obj.texture,
				{ x, y, 0.0f },
				{ ww, hh },
				fullFrame,
				0.25f,
				true
			);

			// Now attach real animations (IDLE/WALK/ATTACK etc.)
			scene.AttachDinoAnimations(g->GetID());
			scene.SetAnimation(g->GetID(), "IDLE");
		}
		else {
			g = scene.SpawnStaticSprite(obj.texture, { x, y, 0.0f }, { ww, hh });
		}


		if (!g) {
			return;
		}

		// Rotation in LEVEL is degrees; GameObject expects radians.
		g->SetRotation(glm::radians(obj.rotation), { 0, 0, 1 });

		// Collider
		g->SetColliderSize({ obj.colWidth, obj.colHeight });
		g->SetColliderOffset({ obj.colOffsetX, obj.colOffsetY });


		// Track texture path for save/inspector
		scene.SetObjectTexturePath(g->GetID(), obj.texture);

		// Tag special handles per-NPC velocity
		if (obj.tag == "player") {
			scene.SetPlayerID(g->GetID());
		}
		else if (obj.tag == "npc1") {
			scene.SetNPC1ID(g->GetID());
		}
		else if (obj.tag == "npc2") {
			scene.SetNPC2ID(g->GetID());
		}
		scene.SetNPCVelocity(g->GetID(), obj.speedX, obj.speedY);

		// Keep editor/scene caches consistent (rotation stays in degrees at editor layer)
		scene.SetTransformFromLevel(g->GetID(),
			{ x, y, 0.0f },
			{ ww, hh, 1.0f },
			obj.rotation);

		// Store defaults so right-click “Reset” works
		Scene::Defaults defs{};
		defs.pos = { x, y, 0.0f };
		defs.size = { ww, hh };
		defs.rot = obj.rotation;                // degrees
		defs.colSize = { obj.colWidth,  obj.colHeight };
		defs.colOff = { obj.colOffsetX, obj.colOffsetY };

		defs.vel = { obj.speedX, obj.speedY };
		defs.texture = obj.texture;
		defs.tag = obj.tag;
		scene.SetDefaults(g->GetID(), defs);

		// Clamp to walk area once after spawn
		scene.ClampToWalkArea(g);
	}
}

static void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
	levelOut.objects.clear();

	std::vector<GameObject*> list;
	scene.CollectRenderablePointers(list);

	for (GameObject* g : list) {
		if (!g) continue;

		LevelObject obj{};
		obj.texture = scene.GetObjectTexturePath(g->GetID());
		obj.rotation = glm::degrees(g->GetRotationAngleZ());
		glm::vec3 p = g->GetPositionGLM();
		glm::vec3 s = g->GetScaleGLM();

		obj.x = p.x;
		obj.y = p.y;
		obj.w = s.x;
		obj.h = s.y;

		obj.tag = "";
		if (g->GetID() == scene.GetPlayerID()) obj.tag = "player";
		else if (g->GetID() == scene.GetNPC1ID()) obj.tag = "npc1";
		else if (g->GetID() == scene.GetNPC2ID()) obj.tag = "npc2";

		obj.animated = scene.HasAnimations(g->GetID());
		glm::vec2 v = scene.GetNPCVelocity(g->GetID());
		obj.speedX = v.x; obj.speedY = v.y;

		levelOut.objects.push_back(obj);
	}
}
