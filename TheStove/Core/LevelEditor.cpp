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

	const float ww = scene.ScaleXToCurrent(prefab.w);
	const float hh = scene.ScaleYToCurrent(prefab.h);
	obj->SetScale(glm::vec3(ww, hh, 1.0f));

	obj->SetColliderSize({
		scene.ScaleXToCurrent(prefab.colWidth),
		scene.ScaleYToCurrent(prefab.colHeight)
		});
	obj->SetColliderOffset({
		scene.ScaleXToCurrent(prefab.colOffsetX),
		scene.ScaleYToCurrent(prefab.colOffsetY)
		});

	// keep position; write current-space transform for editor cache
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

	ImGui::Begin("Level Editor");

	// File Path Row
	static char _pathBuf[256] = "../levels/kitchen01.json";
	if (levelPath.empty()) {
		levelPath = _pathBuf; // one-time init
	}

	ImGui::InputText("Level path", _pathBuf, IM_ARRAYSIZE(_pathBuf));
	levelPath = _pathBuf;

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

		std::vector<GameObject*> objectList;
		scene.CollectRenderablePointers(objectList);

		for (GameObject* obj : objectList) {
			if (!obj) {
				continue;
			}

			LevelObject out{};
			out.texture = scene.GetObjectTexturePath(obj->GetID());

			const glm::vec3 position = obj->GetPositionGLM();
			const glm::vec3 size = obj->GetScaleGLM();

			out.x = scene.ToRefX(position.x);
			out.y = scene.ToRefY(position.y);
			out.z = position.z;
			out.w = scene.ToRefX(size.x);
			out.h = scene.ToRefY(size.y);

			// Save DEGREES to JSON.
			out.rotation = glm::degrees(obj->GetRotationAngleZ());

			const auto colliderSize = obj->GetColliderSize();
			const auto colliderOffset = obj->GetColliderOffset();

			out.colWidth = scene.ToRefX(colliderSize.x);
			out.colHeight = scene.ToRefY(colliderSize.y);
			out.colOffsetX = scene.ToRefX(colliderOffset.x);
			out.colOffsetY = scene.ToRefY(colliderOffset.y);

			// Infer tag from special IDs.
			if (obj->GetID() == scene.GetPlayerID()) { out.tag = "player"; }
			if (obj->GetID() == scene.GetNPC1ID()) { out.tag = "npc1"; }
			if (obj->GetID() == scene.GetNPC2ID()) { out.tag = "npc2"; }
			if (obj->GetID() == scene.GetDinoID()) { out.tag = "dino"; }

			// Optional: persist velocity (already available in Scene).
			const glm::vec2 v = scene.GetNPCVelocity(obj->GetID());
			out.speedX = v.x; out.speedY = v.y;

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

			// scene.ResetResizeBaseline();
			scene.SetSimulationActive(false);

			isPlaying = false;
		}
	}

	ImGui::Separator();

	// Hierarchy
	std::vector<GameObject*> objectList;
	scene.CollectRenderablePointers(objectList);

	if (ImGui::BeginListBox("Objects", ImVec2(-FLT_MIN, 200.0f))) {
		for (int i = 0; i < static_cast<int>(objectList.size()); ++i) {
			if (!objectList[i]) {
				continue;
			}

			std::string label = "ID " + std::to_string(objectList[i]->GetID());
			if (ImGui::Selectable(label.c_str(), selectedIndex == i)) {
				selectedIndex = i;
			}
		}

		ImGui::EndListBox();
	}

	// Property Inspector
	if (selectedIndex >= 0 &&
		selectedIndex < static_cast<int>(objectList.size()) &&
		objectList[selectedIndex])
	{
		// Disable editing while playing
		if (isPlaying) {
			ImGui::BeginDisabled();
		}

		GameObject* obj = objectList[selectedIndex];
		const int id = obj->GetID();

		ImGui::Separator();
		ImGui::Text("Properties (ID %d)", id);

		// Read current values
		char textureBuf[256];
		{
			std::string texPath = scene.GetObjectTexturePath(id);
			if (texPath.empty()) {
				texPath = "../assets/goat_sprite_front.png";
			}

			std::snprintf(textureBuf, sizeof(textureBuf), "%s", texPath.c_str());
		}

		// Infer current tag from special IDs (editable).
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
		glm::vec3 size = obj->GetScaleGLM();// z ignored for sprites
		float rotationDeg = glm::degrees(obj->GetRotationAngleZ());

		auto colliderSize = obj->GetColliderSize();
		auto colliderOffset = obj->GetColliderOffset();
		glm::vec2 velocity = scene.GetNPCVelocity(id);

		// Defaults for right-click reset.
		const auto defaults = scene.GetDefaults(id);

		// Helpers: drag with context "Reset" -----------------------------------
		auto DragVec2WithReset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
			bool changed = ImGui::DragFloat2(label, v, speed);
			if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
				if (ImGui::MenuItem("Reset to default")) {
					v[0] = d.x; v[1] = d.y;
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
			if (ImGui::BeginPopupContextItem((std::string(label) + "_ctx").c_str())) {
				if (ImGui::MenuItem("Reset to default")) {
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

		auto ApplyTransform = [&]() {
			// GameObject expects RADIANS.
			obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });

			// Scene/editor store DEGREES.
			scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
			scene.ClampToWalkArea(obj);
			};

		// Texture (with reset)
		if (ImGui::InputText("Texture", textureBuf, IM_ARRAYSIZE(textureBuf))) {
			scene.SetObjectTexturePath(id, textureBuf);
			if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(textureBuf)), textureBuf)) {
				obj->SetTexture(tex);
			}
		}

		if (ImGui::BeginPopupContextItem("tex_ctx")) {
			if (ImGui::MenuItem("Reset texture")) {
				std::snprintf(textureBuf, sizeof(textureBuf), "%s", defaults.texture.c_str());
				scene.SetObjectTexturePath(id, textureBuf);
				if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(textureBuf)), textureBuf)) {
					obj->SetTexture(tex);
				}
			}

			ImGui::EndPopup();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Right-click to reset");
		}

		// Tag (with reset)
		ImGui::InputText("Tag", tagBuf, IM_ARRAYSIZE(tagBuf));
		if (ImGui::BeginPopupContextItem("tag_ctx")) {
			if (ImGui::MenuItem("Reset tag")) {
				std::snprintf(tagBuf, sizeof(tagBuf), "%s", defaults.tag.c_str());
			}

			ImGui::EndPopup();
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Right-click to reset");
		}

		// Position / Size / Rotation
		DragVec2WithReset("Position", &position.x, ImVec2(defaults.pos.x, defaults.pos.y), 1.0f, [&](bool) { ApplyTransform(); });
		DragVec2WithReset("Size (w,h)", &size.x, ImVec2(defaults.size.x, defaults.size.y), 1.0f, [&](bool) { ApplyTransform(); });
		DragFloatWithReset("Rotation (deg)", &rotationDeg, defaults.rot, 0.25f, [&](bool) { ApplyTransform(); });

		// Collider
		DragVec2WithReset("Collider (w,h)", &colliderSize.x, ImVec2(defaults.colSize.x, defaults.colSize.y), 1.0f,
			[&](bool) { obj->SetColliderSize({ colliderSize.x, colliderSize.y }); });
		DragVec2WithReset("Collider offset", &colliderOffset.x, ImVec2(defaults.colOff.x, defaults.colOff.y), 1.0f,
			[&](bool) { obj->SetColliderOffset({ colliderOffset.x, colliderOffset.y }); });

		// Velocity
		DragVec2WithReset("Velocity (x,y)", &velocity.x, ImVec2(defaults.vel.x, defaults.vel.y), 1.0f,
			[&](bool) { scene.SetNPCVelocity(id, velocity.x, velocity.y); });

		// Animations
		if (scene.HasAnimations(id)) {
			std::vector<std::string> animationNames = scene.GetAnimationList(id);
			std::string currentAnim = scene.GetCurrentAnimationName(id);

			int currentIndex = 0;
			for (int i = 0; i < static_cast<int>(animationNames.size()); ++i) {
				if (animationNames[i] == currentAnim) {
					currentIndex = i;
					break;
				}
			}

			if (ImGui::BeginCombo("Start animation", currentAnim.empty() ? "(none)" : currentAnim.c_str())) {
				for (int i = 0; i < static_cast<int>(animationNames.size()); ++i) {
					bool selected = (i == currentIndex);
					if (ImGui::Selectable(animationNames[i].c_str(), selected)) {
						scene.SetAnimation(id, animationNames[i]); // switch THIS object's animation
					}

					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginPopupContextItem("anim_ctx")) {
				if (ImGui::MenuItem("Reset animation")) {
					scene.SetAnimation(id, "IDLE");
				}

				ImGui::EndPopup();
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Right-click to reset");
			}
		}

		// Final apply (keep maps in sync)
		scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
		obj->SetColliderSize({ colliderSize.x, colliderSize.y });
		obj->SetColliderOffset({ colliderOffset.x, colliderOffset.y });
		scene.SetNPCVelocity(id, velocity.x, velocity.y);

		// Update special IDs if tag changed.
		std::string newTag = tagBuf;
		if (newTag == "player") { scene.SetPlayerID(id); }
		if (newTag == "npc1") { scene.SetNPC1ID(id); }
		if (newTag == "npc2") { scene.SetNPC2ID(id); }
		if (newTag == "dino") { scene.SetDinoID(id); }

		if (isPlaying) {
			ImGui::EndDisabled();
		}
	}

	// --- Prefabs / Archetypes ----------------------------------------------------
	ImGui::Separator();
	ImGui::Text("Prefabs / Archetypes");

	// Choose a prefab path (you can change it per use)
	static char prefabPathBuf[256] = "../prefabs/my_goat.json";
	ImGui::InputText("Prefab path", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

	if (ImGui::Button("Save selected as prefab")) {
		if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objectList.size()) && objectList[selectedIndex]) {
			GameObject* obj = objectList[selectedIndex];

			LevelObject data{};
			data.texture = scene.GetObjectTexturePath(obj->GetID());
			const glm::vec3 p = obj->GetPositionGLM();
			const glm::vec3 s = obj->GetScaleGLM();
			data.x = p.x; data.y = p.y; data.z = p.z;
			data.w = s.x; data.h = s.y;
			data.rotation = glm::degrees(obj->GetRotationAngleZ());

			const auto csz = obj->GetColliderSize();
			const auto cof = obj->GetColliderOffset();
			data.colWidth = csz.x; data.colHeight = csz.y;
			data.colOffsetX = cof.x; data.colOffsetY = cof.y;

			// Optional: tag/velocity persist
			data.tag = "prefab";
			const glm::vec2 v = scene.GetNPCVelocity(obj->GetID());
			data.speedX = v.x; data.speedY = v.y;

			if (SavePrefabToFile(prefabPathBuf, data)) {
				// Remember link for propagation
				prefabPathById[obj->GetID()] = prefabPathBuf;
			}
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Instantiate from prefab")) {
		LevelObject data{};
		if (LoadPrefabFromFile(prefabPathBuf, data)) {
			const float x = scene.ScaleXToCurrent(data.x);
			const float y = scene.ScaleYToCurrent(data.y);
			const float ww = scene.ScaleXToCurrent(data.w);
			const float hh = scene.ScaleYToCurrent(data.h);

			GameObject* obj = scene.SpawnStaticSprite(
				data.texture, { x, y, data.z }, { ww, hh });

			if (obj) {
				obj->SetRotation(glm::radians(data.rotation), { 0,0,1 });
				obj->SetColliderSize({
					scene.ScaleXToCurrent(data.colWidth),
					scene.ScaleYToCurrent(data.colHeight)
					});
				obj->SetColliderOffset({
					scene.ScaleXToCurrent(data.colOffsetX),
					scene.ScaleYToCurrent(data.colOffsetY)
					});
				scene.SetObjectTexturePath(obj->GetID(), data.texture);

				scene.SetTransformFromLevel(
					obj->GetID(),
					{ x, y, data.z },
					{ ww, hh, 1.0f },
					data.rotation
				);
				scene.ClampToWalkArea(obj);
			}
		}
	}

	if (ImGui::Button("Propagate prefab changes")) {
		LevelObject data{};
		if (LoadPrefabFromFile(prefabPathBuf, data)) {
			std::vector<GameObject*> all;
			scene.CollectRenderablePointers(all);
			for (GameObject* g : all) {
				if (!g) { continue; }
				auto it = prefabPathById.find(g->GetID());
				if (it != prefabPathById.end() && it->second == std::string(prefabPathBuf)) {
					ApplyPrefabToObjectKeepPosition(data, scene, g);
				}
			}
		}
	}

	if (isPlaying) {
		ImGui::BeginDisabled();
	}

	// Add / Remove
	if (ImGui::Button("Add Object")) {
		LevelObject proto{};
		proto.texture = "../assets/goat_sprite_front.png";
		proto.tag = "npc"; // or "" if none
		proto.x = 300.0f; proto.y = 300.0f; proto.z = 0.0f;
		proto.w = 128.0f; proto.h = 128.0f; proto.rotation = 0.0f;

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
	if (ImGui::Button("Remove Selected") &&
		selectedIndex >= 0 &&
		selectedIndex < static_cast<int>(objectList.size()))
	{
		scene.DespawnByID(objectList[selectedIndex]->GetID());
		selectedIndex = -1;
	}

	if (isPlaying) {
		ImGui::EndDisabled();
	}

	ImGui::End();
}

static void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
	for (const auto& obj : levelIn.objects) {
		const float x = scene.ScaleXToCurrent(obj.x);
		const float y = scene.ScaleYToCurrent(obj.y);
		const float ww = scene.ScaleXToCurrent(obj.w);
		const float hh = scene.ScaleYToCurrent(obj.h);

		GameObject* g = nullptr;
		if (obj.animated) {
			g = scene.SpawnAnimatedSprite(
				obj.texture,
				{ x, y, 0.0f },
				{ ww, hh },
				/*frames*/{}, 0.2f, true);
		}
		else {
			g = scene.SpawnStaticSprite(
				obj.texture,
				{ x, y, 0.0f },
				{ ww, hh });
		}

		if (!g) {
			return;
		}

		// Rotation in LEVEL is degrees; GameObject expects radians.
		g->SetRotation(glm::radians(obj.rotation), { 0, 0, 1 });

		// Collider
		g->SetColliderSize({ scene.ScaleXToCurrent(obj.colWidth), scene.ScaleYToCurrent(obj.colHeight) });
		g->SetColliderOffset({ scene.ScaleXToCurrent(obj.colOffsetX), scene.ScaleYToCurrent(obj.colOffsetY) });

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
		defs.colSize = { scene.ScaleXToCurrent(obj.colWidth),  scene.ScaleYToCurrent(obj.colHeight) };
		defs.colOff = { scene.ScaleXToCurrent(obj.colOffsetX), scene.ScaleYToCurrent(obj.colOffsetY) };
		defs.vel = { obj.speedX, obj.speedY };
		defs.texture = obj.texture;
		defs.tag = obj.tag;
		scene.SetDefaults(g->GetID(), defs);

		// Optional: attach a standard animation set to animated objects
		if (obj.animated) {
			scene.AttachDinoAnimations(g->GetID());
		}

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

		obj.x = scene.ToRefX(p.x);
		obj.y = scene.ToRefY(p.y);
		obj.w = scene.ToRefX(s.x);
		obj.h = scene.ToRefY(s.y);

		obj.tag = "";
		if (g->GetID() == scene.GetPlayerID()) obj.tag = "player";
		else if (g->GetID() == scene.GetNPC1ID()) obj.tag = "npc1";
		else if (g->GetID() == scene.GetNPC2ID()) obj.tag = "npc2";

		glm::vec2 v = scene.GetNPCVelocity(g->GetID());
		obj.speedX = v.x; obj.speedY = v.y;

		levelOut.objects.push_back(obj);
	}
}
