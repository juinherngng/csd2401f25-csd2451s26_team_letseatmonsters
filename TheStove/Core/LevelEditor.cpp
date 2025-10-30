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

				scene.SetTransformFromLevel(
					g->GetID(),
					{ o.x, o.y, o.z },
					{ o.w, o.h, 1.0f },
					o.rotation
				);

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

				if (o.tag == "dino") {
					scene.SetDinoID(g->GetID());
					scene.AttachDinoAnimations(g->GetID());   // <-- only for dinos
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
