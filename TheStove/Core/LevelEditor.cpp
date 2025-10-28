#include "LevelEditor.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
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
	levelPath_ = "../levels/kitchen01.json";

	if (ImGui::Button("Load Level")) {
		if (LevelSerializer::Load(levelPath_, level_)) {
			// clear opt., then spawn
			for (auto& o : level_.objects) {
				auto* g = scene.SpawnStaticSprite(o.texture, { o.x,o.y,0 }, { o.w,o.h });
				if (g) scene.SetObjectTexturePath(g->GetID(), o.texture);
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Save Level")) {
		// snapshot scene -> level_
		level_.objects.clear();
		std::vector<GameObject*> objs; scene.CollectRenderablePointers(objs);
		for (auto* g : objs) {
			if (!g) continue;
			LevelObject o;
			auto p = g->GetPositionGLM();
			auto s = g->GetScaleGLM();
			o.x = p.x; o.y = p.y; o.w = s.x; o.h = s.y;
			o.rotation = g->GetRotationAngleZ();
			o.texture = scene.GetObjectTexturePath(g->GetID());
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
		LevelObject o{ "../assets/goat_sprite_front.png", 300,300,128,128,0 };
		auto* g = scene.SpawnStaticSprite(o.texture, { o.x,o.y,0 }, { o.w,o.h });
		if (g) scene.SetObjectTexturePath(g->GetID(), o.texture);
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove Selected") && sel >= 0 && sel < (int)objs.size() && objs[sel]) {
		scene.DespawnByID(objs[sel]->GetID());
		sel = -1;
	}

	ImGui::End();

}

// -------- helpers: sync scene <-> LevelData ---------------------------------
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
