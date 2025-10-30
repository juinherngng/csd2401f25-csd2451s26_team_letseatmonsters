/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			LevelEditor.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "LevelEditor.hpp"
#include "imgui.h"
#include "imgui_internal.h"

static void SyncSceneToLevel(Scene& scene, LevelData& lvl);
static void SyncLevelToScene(const LevelData& lvl, Scene& scene);

bool LevelEditor::LoadIntoScene(Scene& scene) {
	if (!LevelSerializer::Load(levelPath_, level_)) return false;
	SyncLevelToScene(level_, scene);
	return true;
}

void LevelEditor::DrawUI(Scene& scene) {
	if (!enabled_) return;

	ImGuiContext* ctx = ImGui::GetCurrentContext();
	if (!ctx || !ctx->WithinFrameScope) {
		return; // ImGui frame hasn’t started yet; skip safely
	}

	bool open = true;
	if (ImGui::Begin("Level Editor", &open)) {
		// ... your UI ...
	}
	ImGui::End();

	if (!open) enabled_ = false;

	ImGui::Begin("Level Editor");

	// editable level path once
	static char pathBuf[256] = "../levels/kitchen01.json";
	if (levelPath_.empty()) levelPath_ = pathBuf; // init once
	ImGui::InputText("Level path", pathBuf, IM_ARRAYSIZE(pathBuf));
	levelPath_ = pathBuf;

	if (ImGui::Button("Load Level")) {
		if (LevelSerializer::Load(levelPath_, level_)) {
			// start clean so the scene only reflects the file
			scene.ClearAll();

			for (auto& o : level_.objects) {
				GameObject* g = nullptr;
				if (o.animated) {
					g = scene.SpawnAnimatedSprite(o.texture, { o.x, o.y, o.z }, { o.w, o.h }, /*frames*/{}, 0.2f, true);
				}
				else {
					g = scene.SpawnStaticSprite(o.texture, { o.x, o.y, o.z }, { o.w, o.h });
				}
				if (!g) continue;

				g->SetRotation(o.rotation, { 0,0,1 });
				g->SetColliderSize({ o.col_w, o.col_h });
				g->SetColliderOffset({ o.col_offx, o.col_offy });
				scene.SetObjectTexturePath(g->GetID(), o.texture);

				Scene::Defaults d;
				d.pos = { o.x, o.y, o.z };
				d.size = { o.w, o.h };
				d.rot = o.rotation;
				d.colSize = { o.col_w, o.col_h };
				d.colOff = { o.col_offx, o.col_offy };
				d.vel = { 0.f, o.speed_y };
				d.texture = o.texture;
				d.tag = o.tag;
				scene.SetDefaults(g->GetID(), d);

				scene.SetTransformFromLevel(
					g->GetID(),
					{ o.x, o.y, o.z },
					{ o.w, o.h, 1.0f },
					o.rotation
				);

				scene.ClampToWalkArea(g);

				// assign special IDs by tag
				if (o.tag == "player") scene.SetPlayerID(g->GetID());
				if (o.tag == "npc1") {
					scene.SetNPC1ID(g->GetID());
					scene.SetNPCVelocity(g->GetID(), 0.f, o.speed_y); // JSON drives Y speed
				}
				if (o.tag == "npc2") {
					scene.SetNPC2ID(g->GetID());
					scene.SetNPCVelocity(g->GetID(), 0.f, o.speed_y);
				}
				if (o.animated) {
					scene.AttachDinoAnimations(g->GetID());   // give this object its own IDLE/WALK/ATTACK set
				}
			}
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Save Level")) {
		level_.objects.clear();
		std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);
		for (auto* g : objs) {
			if (!g) continue;

			LevelObject o;
			o.texture = scene.GetObjectTexturePath(g->GetID());
			const auto p = g->GetPositionGLM();
			const auto s = g->GetScaleGLM();
			o.x = p.x; o.y = p.y; o.z = p.z;
			o.w = s.x; o.h = s.y;
			o.rotation = g->GetRotationAngleZ();

			const auto cs = g->GetColliderSize();
			const auto co = g->GetColliderOffset();
			o.col_w = cs.x; o.col_h = cs.y;
			o.col_offx = co.x; o.col_offy = co.y;

			// infer tag for special IDs
			if (g->GetID() == scene.GetPlayerID()) o.tag = "player";
			if (g->GetID() == scene.GetNPC1ID())   o.tag = "npc1";
			if (g->GetID() == scene.GetNPC2ID())   o.tag = "npc2";
			if (g->GetID() == scene.GetDinoID())   o.tag = "dino";

			// (Optional) if you add velocity storage:
			// auto v = scene.GetVelocityFor(g->GetID());
			// o.speed_x = v.x; o.speed_y = v.y;

			level_.objects.push_back(o);
		}
		LevelSerializer::Save(levelPath_, level_);
	}

	ImGui::Separator();

	// Hierarchy
	std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);
	static int sel = -1;
	if (ImGui::BeginListBox("Objects", ImVec2(-FLT_MIN, 200))) {
		for (int i = 0;i < (int)objs.size();++i) {
			if (!objs[i]) continue;
			std::string label = "ID " + std::to_string(objs[i]->GetID());
			if (ImGui::Selectable(label.c_str(), sel == i)) sel = i;
		}
		ImGui::EndListBox();
	}

	// --- Property Editor --------------------------------------------------------
	if (sel >= 0 && sel < static_cast<int>(objs.size()) && objs[sel]) {
		GameObject* g = objs[sel];
		const int id = g->GetID();

		ImGui::Separator();
		ImGui::Text("Properties (ID %d)", id);

		// ---------- Read current values ----------
		char texBuf[256];
		{
			std::string tex = scene.GetObjectTexturePath(id);
			if (tex.empty()) tex = "../assets/goat_sprite_front.png";
			std::snprintf(texBuf, sizeof(texBuf), "%s", tex.c_str());
		}

		// infer current tag from special IDs (editable)
		char tagBuf[64] = "";
		if (id == scene.GetPlayerID()) std::snprintf(tagBuf, sizeof(tagBuf), "player");
		else if (id == scene.GetNPC1ID())   std::snprintf(tagBuf, sizeof(tagBuf), "npc1");
		else if (id == scene.GetNPC2ID())   std::snprintf(tagBuf, sizeof(tagBuf), "npc2");
		else if (id == scene.GetDinoID())   std::snprintf(tagBuf, sizeof(tagBuf), "dino");

		glm::vec3 pos = g->GetPositionGLM();
		glm::vec3 scl = g->GetScaleGLM();          // z ignored for sprites
		float     rotation = g->GetRotationAngleZ();

		auto cs = g->GetColliderSize();
		auto co = g->GetColliderOffset();

		glm::vec2 vel = scene.GetNPCVelocity(id);

		// ---------- Defaults for right-click reset ----------
		const auto def = scene.GetDefaults(id);

		// Helpers: drag with context "Reset"
		auto Drag2Reset = [&](const char* label, float* v, ImVec2 d, float speed, auto apply) {
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
		auto Drag1Reset = [&](const char* label, float* v, float d, float speed, auto apply) {
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
		auto ApplyTransform = [&]() {
			scene.SetTransformFromLevel(id, pos, { scl.x, scl.y, 1.0f }, rotation);
			scene.ClampToWalkArea(g);
			};

		// ---------- Texture (with reset) ----------
		if (ImGui::InputText("Texture", texBuf, IM_ARRAYSIZE(texBuf))) {
			scene.SetObjectTexturePath(id, texBuf);
			if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(texBuf)), texBuf))
				g->SetTexture(tex);
		}
		if (ImGui::BeginPopupContextItem("tex_ctx")) {
			if (ImGui::MenuItem("Reset texture")) {
				std::snprintf(texBuf, sizeof(texBuf), "%s", def.texture.c_str());
				scene.SetObjectTexturePath(id, texBuf);
				if (auto* tex = ResourceManager::Instance().LoadTexture(("sprite_" + std::string(texBuf)), texBuf))
					g->SetTexture(tex);
			}
			ImGui::EndPopup();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click to reset");

		// ---------- Tag (with reset) ----------
		ImGui::InputText("Tag", tagBuf, IM_ARRAYSIZE(tagBuf));
		if (ImGui::BeginPopupContextItem("tag_ctx")) {
			if (ImGui::MenuItem("Reset tag")) {
				std::snprintf(tagBuf, sizeof(tagBuf), "%s", def.tag.c_str());
			}
			ImGui::EndPopup();
		}
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click to reset");

		// ---------- Position / Size / Rotation ----------
		Drag2Reset("Position", &pos.x, ImVec2(def.pos.x, def.pos.y), 1.0f, [&](bool) { ApplyTransform(); });
		Drag2Reset("Size (w,h)", &scl.x, ImVec2(def.size.x, def.size.y), 1.0f, [&](bool) { ApplyTransform(); });
		Drag1Reset("Rotation (deg)", &rotation, def.rot, 0.25f, [&](bool) { ApplyTransform(); });

		// ---------- Collider ----------
		Drag2Reset("Collider (w,h)", &cs.x, ImVec2(def.colSize.x, def.colSize.y), 1.0f, [&](bool) {
			g->SetColliderSize({ cs.x, cs.y });
			});
		Drag2Reset("Collider offset", &co.x, ImVec2(def.colOff.x, def.colOff.y), 1.0f, [&](bool) {
			g->SetColliderOffset({ co.x, co.y });
			});

		// ---------- Velocity ----------
		Drag2Reset("Velocity (x,y)", &vel.x, ImVec2(def.vel.x, def.vel.y), 1.0f, [&](bool) {
			scene.SetNPCVelocity(id, vel.x, vel.y);
			});

		// Dinos: choose animation (optional reset to IDLE if you want)
		if (scene.HasAnimations(id)) {
			// read the list and current name for THIS object
			std::vector<std::string> names = scene.GetAnimationList(id);
			std::string cur = scene.GetCurrentAnimationName(id);

			// compute the currently selected index
			int sel = 0;
			for (int i = 0; i < (int)names.size(); ++i) {
				if (names[i] == cur) { sel = i; break; }
			}

			// ImGui combo: each object gets its own selection
			if (ImGui::BeginCombo("Start animation", cur.empty() ? "(none)" : cur.c_str())) {
				for (int i = 0; i < (int)names.size(); ++i) {
					bool selected = (i == sel);
					if (ImGui::Selectable(names[i].c_str(), selected)) {
						scene.SetAnimation(id, names[i]);   // switch THIS object's animation
					}
					if (selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// Optional: right-click reset to "IDLE"
			if (ImGui::BeginPopupContextItem("anim_ctx")) {
				if (ImGui::MenuItem("Reset animation")) {
					scene.SetAnimation(id, "IDLE");
				}
				ImGui::EndPopup();
			}
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Right-click to reset");
		}


		// ---------- Final apply (keep maps in sync) ----------
		scene.SetTransformFromLevel(id, pos, { scl.x, scl.y, 1.0f }, rotation);
		g->SetColliderSize({ cs.x, cs.y });
		g->SetColliderOffset({ co.x, co.y });
		scene.SetNPCVelocity(id, vel.x, vel.y);

		// Update special IDs if tag changed
		std::string newTag = tagBuf;
		if (newTag == "player") scene.SetPlayerID(id);
		if (newTag == "npc1")   scene.SetNPC1ID(id);
		if (newTag == "npc2")   scene.SetNPC2ID(id);
		if (newTag == "dino")   scene.SetDinoID(id);
	}

	// Add / Remove
	if (ImGui::Button("Add Object")) {
		LevelObject o{};                  // value-init defaults
		o.texture = "../assets/goat_sprite_front.png";
		o.tag = "npc";               // or "" if none
		o.x = 300.f;  o.y = 300.f;  o.z = 0.f;
		o.w = 128.f;  o.h = 128.f;  o.rotation = 0.f;
		// o.col_w, o.col_h, offsets keep defaults unless you want to set them

		auto* g = scene.SpawnStaticSprite(o.texture, { o.x, o.y, o.z }, { o.w, o.h });
		if (g) {
			g->SetColliderSize({ o.col_w, o.col_h });
			g->SetColliderOffset({ o.col_offx, o.col_offy });
			scene.SetObjectTexturePath(g->GetID(), o.texture);
		}

		Scene::Defaults d;
		d.pos = { o.x, o.y, o.z };
		d.size = { o.w, o.h };
		d.rot = o.rotation;
		d.colSize = { o.col_w, o.col_h };
		d.colOff = { o.col_offx, o.col_offy };
		d.vel = { 0.f, o.speed_y };
		d.texture = o.texture;
		d.tag = o.tag;
		scene.SetDefaults(g->GetID(), d);
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove Selected") && sel >= 0 && sel < static_cast<int>(objs.size())) {
		scene.DespawnByID(objs[sel]->GetID());
		sel = -1;
	}

	ImGui::End();
}

// helpers: sync scene to LevelData
static void SyncLevelToScene(const LevelData& lvl, Scene& scene) {
	// clear is optional — if you want to keep existing, remove this
	// (You already have DespawnByID; add Scene::ClearAll() if needed.)
	for (auto* g : scene.GetAllObjectsRaw()) { (void)g; } // Keep or clear depending on design

	for (auto& o : lvl.objects) {
		GameObject* g = scene.SpawnStaticSprite(o.texture, { o.x,o.y,0 }, { o.w,o.h });
		if (g) {
			g->SetRotation(o.rotation, { 0,0,1 });
			scene.SetObjectTexturePath(g->GetID(), o.texture);
		}
	}
}

static void SyncSceneToLevel(Scene& scene, LevelData& lvl) {
	lvl.objects.clear();
	std::vector<GameObject*> objs;
	scene.CollectRenderablePointers(objs);
	for (auto* g : objs) {
		if (!g) continue;
		LevelObject o;
		o.texture = scene.GetObjectTexturePath(g->GetID());
		glm::vec3 p = g->GetPositionGLM();
		glm::vec3 s = g->GetScaleGLM();
		o.x = p.x; o.y = p.y; o.w = s.x; o.h = s.y;
		o.rotation = g->GetRotationAngleZ();
		lvl.objects.push_back(o);
	}
}
