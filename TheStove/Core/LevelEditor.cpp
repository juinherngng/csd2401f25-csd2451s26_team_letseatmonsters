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

static void SyncSceneToLevel(Scene& scene, LevelData& levelOut);
static void SyncLevelToScene(const LevelData& levelIn, Scene& scene);

// LevelEditor methods
bool LevelEditor::LoadIntoScene(Scene& scene)
{
	if (!LevelSerializer::Load(levelPath, level)) {
		return false;
	}

	SyncLevelToScene(level, scene);
	return true;
}

void LevelEditor::DrawUI(Scene& scene) {
	if (!isEnabled) {
		return;
	}

	ImGuiContext* imguiContext = ImGui::GetCurrentContext();
	if (imguiContext == nullptr || !imguiContext->WithinFrameScope) {
		// ImGui frame hasn’t started yet; skip safely.
		return;
	}

	bool windowOpen = true;
	if (ImGui::Begin("Level Editor", &windowOpen)) {
		// Header space (kept intentionally minimal).
	}
	ImGui::End();

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

	//  Load 
	if (ImGui::Button("Load Level")) {
		if (LevelSerializer::Load(levelPath, level)) {
			// Start clean so the scene only reflects the file.
			scene.ClearAll();

			for (auto& levelObj : level.objects) {
				GameObject* obj = nullptr;
				if (levelObj.animated) {
					obj = scene.SpawnAnimatedSprite(
						levelObj.texture,
						{ levelObj.x, levelObj.y, levelObj.z },
						{ levelObj.w, levelObj.h },
						/*frames*/{}, 0.2f, true
					);
				}
				else {
					obj = scene.SpawnStaticSprite(
						levelObj.texture,
						{ levelObj.x, levelObj.y, levelObj.z },
						{ levelObj.w, levelObj.h }
					);
				}

				if (!obj) {
					continue;
				}

				// NOTE: Rotation in levelObj.rotation is DEGREES at the editor/JSON layer.
				// Convert to radians at GameObject boundary if your SetRotation expects radians.
				obj->SetRotation(glm::radians(levelObj.rotation), { 0, 0, 1 });

				obj->SetColliderSize({ levelObj.colWidth, levelObj.colHeight });
				obj->SetColliderOffset({ levelObj.colOffsetX, levelObj.colOffsetY });
				scene.SetObjectTexturePath(obj->GetID(), levelObj.texture);

				// Store defaults for right click reset in the inspector.
				Scene::Defaults defaults{};
				defaults.pos = { levelObj.x, levelObj.y, levelObj.z };
				defaults.size = { levelObj.w, levelObj.h };
				defaults.rot = levelObj.rotation; // degrees
				defaults.colSize = { levelObj.colWidth, levelObj.colHeight };
				defaults.colOff = { levelObj.colOffsetX, levelObj.colOffsetY };
				defaults.vel = { levelObj.speedX, levelObj.speedY };
				defaults.texture = levelObj.texture;
				defaults.tag = levelObj.tag;
				scene.SetDefaults(obj->GetID(), defaults);

				// Keep editor/scene state in sync (rotation in DEGREES).
				scene.SetTransformFromLevel(
					obj->GetID(),
					{ levelObj.x, levelObj.y, levelObj.z },
					{ levelObj.w, levelObj.h, 1.0f },
					levelObj.rotation
				);

				scene.ClampToWalkArea(obj);

				// Named handles by tag.
				if (levelObj.tag == "player") {
					scene.SetPlayerID(obj->GetID());
				}
				if (levelObj.tag == "npc1") {
					scene.SetNPC1ID(obj->GetID());
					scene.SetNPCVelocity(obj->GetID(), levelObj.speedX, levelObj.speedY);
				}
				if (levelObj.tag == "npc2") {
					scene.SetNPC2ID(obj->GetID());
					scene.SetNPCVelocity(obj->GetID(), levelObj.speedX, levelObj.speedY);
				}

				if (levelObj.animated) {
					// Give this object its own IDLE/WALK/ATTACK set.
					scene.AttachDinoAnimations(obj->GetID());
				}
			}
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

			out.x = position.x;
			out.y = position.y;
			out.z = position.z;
			out.w = size.x;
			out.h = size.y;

			// Save DEGREES to JSON.
			out.rotation = glm::degrees(obj->GetRotationAngleZ());

			const auto colliderSize = obj->GetColliderSize();
			const auto colliderOffset = obj->GetColliderOffset();
			out.colWidth = colliderSize.x;
			out.colHeight = colliderSize.y;
			out.colOffsetX = colliderOffset.x;
			out.colOffsetY = colliderOffset.y;

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

	ImGui::Separator();

	// Hierarchy
	std::vector<GameObject*> objectList;
	scene.CollectRenderablePointers(objectList);

	static int selectedIndex = -1;
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

	ImGui::End();
}

// Optional sync helpers (kept for parity)
static void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
	// If you prefer: scene.ClearAll();
	for (auto* g : scene.GetAllObjectsRaw()) {
		(void)g; // Keep or clear depending on design
	}

	for (const auto& levelObj : levelIn.objects) {
		GameObject* obj = scene.SpawnStaticSprite(levelObj.texture, { levelObj.x, levelObj.y, 0.0f }, { levelObj.w, levelObj.h });
		if (obj) {
			obj->SetRotation(glm::radians(levelObj.rotation), { 0, 0, 1 }); // JSON/editor is degrees
			scene.SetObjectTexturePath(obj->GetID(), levelObj.texture);
		}
	}
}

static void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
	levelOut.objects.clear();

	std::vector<GameObject*> objectList;
	scene.CollectRenderablePointers(objectList);

	for (GameObject* obj : objectList) {
		if (!obj) {
			continue;
		}

		LevelObject out{};
		out.texture = scene.GetObjectTexturePath(obj->GetID());

		glm::vec3 p = obj->GetPositionGLM();
		glm::vec3 s = obj->GetScaleGLM();

		out.x = p.x; out.y = p.y;
		out.w = s.x; out.h = s.y;

		// Store DEGREES.
		out.rotation = glm::degrees(obj->GetRotationAngleZ());

		levelOut.objects.push_back(out);
	}
}
