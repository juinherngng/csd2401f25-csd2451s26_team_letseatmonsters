/*
----------------------------------------------------------------------------------------------------
FILE NAME:			LevelEditorPanelLevel.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Yat Chun Wee, y.chunwee@digipen.edu		(70%)
CO-AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (5%)
					Vu Phan Hung, phanhung.vu@digipen.edu	(25%)

DESCRIPTION:       Implementation of the Level panel.
					- Load/Save levels to JSON
					- Play/Stop scene simulation
					- Hierarchy list and object inspector
					- Add/Remove objects
					- Drag-drop prefab/texture instantiation
					- Keeps LevelData synchronized with Scene state

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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
#include <filesystem>
#include <windows.h>

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/Layer.hpp"

#include "LevelEditorPanelLevel.hpp"
#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "LevelEditorPrefabLinks.hpp"
#include "LevelEditorPanelFonts.hpp"  // Include for text object sync
#include "InputManager.hpp"
#include "AudioLoading.hpp"


namespace fs = std::filesystem;

using namespace LEFILEIO;

#ifdef _DEBUG
static std::filesystem::path GetExeDir() {
	char buf[MAX_PATH]{};
	GetModuleFileNameA(nullptr, buf, MAX_PATH);
	return std::filesystem::path(buf).parent_path();
}

static std::filesystem::path FindRepoRoot() {
	namespace fs = std::filesystem;
	fs::path p = GetExeDir();

	// Walk upwards until we find BOTH build/ and levels/
	for (int i = 0; i < 10; ++i) {
		if (fs::exists(p / "build") && fs::exists(p / "levels")) {
			return p;
		}

		if (!p.has_parent_path()) {
			break;
		}

		p = p.parent_path();
	}

	// Fallback (should not happen)
	return fs::current_path();
}
#endif

namespace {
	// Internal helpers for Level <-> Scene synchronization
	void SyncLevelToScene(const LevelData& levelIn, Scene& scene);
	void SyncSceneToLevel(Scene& scene, LevelData& levelOut);

	// Helper to sync text objects from LevelData to editor state
	void SyncTextObjectsToEditor(const LevelData& levelIn) {
		std::vector<LEPANELFONTS::TextObjectData> textObjects;
		textObjects.reserve(levelIn.textObjects.size());

		for (const auto& levelText : levelIn.textObjects) {
			LEPANELFONTS::TextObjectData textData;
			textData.name = levelText.name;
			textData.fontName = levelText.fontName;
			textData.text = levelText.text;
			textData.x = levelText.x;
			textData.y = levelText.y;
			textData.scale = levelText.scale;
			textData.rotation = levelText.rotation;
			textData.useBlockRotation = levelText.useBlockRotation;
			textData.colorR = levelText.colorR;
			textData.colorG = levelText.colorG;
			textData.colorB = levelText.colorB;
			textData.colorA = levelText.colorA;
			textData.layer = levelText.layer;
			textData.visible = levelText.visible;

			// Try to load the font if not already loaded
			if (!levelText.fontName.empty()) {
				FontSystem::Font* font = ResourceManager::Instance().GetFont(levelText.fontName);
				if (!font) {
					// Try to load with a default path - this won't work without the actual path
					// In practice, fonts should be pre-loaded or the path should be stored
					std::cout << "[SyncTextObjects] Font '" << levelText.fontName << "' not loaded, text may not render\n";
				}
			}

			textObjects.push_back(textData);
		}

		LEPANELFONTS::SetTextObjects(textObjects);
	}

	// NEW: Helper to sync text objects from editor state to LevelData
	void SyncTextObjectsToLevel(LevelData& levelOut) {
		const auto& textObjects = LEPANELFONTS::GetTextObjects();
		levelOut.textObjects.clear();
		levelOut.textObjects.reserve(textObjects.size());

		for (const auto& textData : textObjects) {
			LevelTextObject levelText;
			levelText.name = textData.name;
			levelText.fontName = textData.fontName;
			levelText.text = textData.text;
			levelText.x = textData.x;
			levelText.y = textData.y;
			levelText.scale = textData.scale;
			levelText.rotation = textData.rotation;
			levelText.useBlockRotation = textData.useBlockRotation;
			levelText.colorR = textData.colorR;
			levelText.colorG = textData.colorG;
			levelText.colorB = textData.colorB;
			levelText.colorA = textData.colorA;
			levelText.layer = textData.layer;
			levelText.visible = textData.visible;

			// Get font size from loaded font if available
			FontSystem::Font* font = ResourceManager::Instance().GetFont(textData.fontName);
			if (font) {
				levelText.fontSize = font->GetFontSize();
			}

			levelOut.textObjects.push_back(levelText);
		}
	}

#ifdef _DEBUG
	static constexpr int MAX_UNDO = 50;
	static std::vector<LevelData> sUndoStack;
	static std::vector<LevelData> sRedoStack;

	// Take a snapshot of the current Scene into LevelData and push onto the stack.
	static void PushUndoSnapshot(LevelEditor& editor, Scene& scene) {
		if (editor.IsPlaying()) {
			return;
		}

		LevelData snap{};
		SyncSceneToLevel(scene, snap);
		SyncTextObjectsToLevel(snap);

		sUndoStack.push_back(snap);
		if (sUndoStack.size() > MAX_UNDO) {
			sUndoStack.erase(sUndoStack.begin());
		}

		// New user action invalidates redo history
		sRedoStack.clear();

		// Keep the editor's working LevelData in sync with the scene
		editor.MutableLevel() = snap;
	}

	// Pop last snapshot and restore it into the Scene.
	static bool PerformUndo(LevelEditor& editor, Scene& scene) {
		if (editor.IsPlaying() || sUndoStack.empty()) {
			return false;
		}

		// Capture current state as the target for redo
		LevelData current{};
		SyncSceneToLevel(scene, current);
		SyncTextObjectsToLevel(current);
		sRedoStack.push_back(current);
		if (sRedoStack.size() > MAX_UNDO) {
			sRedoStack.erase(sRedoStack.begin());
		}

		LevelData snap = sUndoStack.back();
		sUndoStack.pop_back();

		scene.ClearAll();
		SyncLevelToScene(snap, scene);
		scene.RebuildColliders();
		scene.SetSimulationActive(false);

		editor.SetPlaying(false);
		editor.MutableLevel() = snap;
		SyncTextObjectsToEditor(snap);

		return true;
	}

	// Pop last redo snapshot and restore it into the Scene.
	static bool PerformRedo(LevelEditor& editor, Scene& scene) {
		if (editor.IsPlaying() || sRedoStack.empty()) {
			return false;
		}

		// Current state becomes new undo point
		LevelData current{};
		SyncSceneToLevel(scene, current);
		SyncTextObjectsToLevel(current);
		sUndoStack.push_back(current);
		if (sUndoStack.size() > MAX_UNDO) {
			sUndoStack.erase(sUndoStack.begin());
		}

		LevelData snap = sRedoStack.back();
		sRedoStack.pop_back();

		scene.ClearAll();
		SyncLevelToScene(snap, scene);
		scene.RebuildColliders();
		scene.SetSimulationActive(false);

		editor.SetPlaying(false);
		editor.MutableLevel() = snap;
		SyncTextObjectsToEditor(snap);

		return true;
	}
#endif // _DEBUG
	static void ClearUndoHistory() {
		sUndoStack.clear();
		sRedoStack.clear();
	}

	// Build the current scene from loaded LevelData.
	void SyncLevelToScene(const LevelData& levelIn, Scene& scene) {
		LELINKS::PrefabLinkByID.clear();

		for (const auto& obj : levelIn.objects) {
			GameObject* g = nullptr;

			// Use "Default" when the saved layer name is empty
			std::string layerName = obj.layer.empty() ? "1" : obj.layer;

			// Spawn animated or static
			if (obj.animated) {
				const std::vector<glm::vec4> fullFrame = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
				g = scene.SpawnAnimatedSprite(obj.texture, { obj.x, obj.y, obj.z }, { obj.w, obj.h },
					fullFrame, 0.25f, true, layerName);

				if (obj.texture.find("dino") != std::string::npos) {
					scene.AttachDinoAnimations(g->GetID());

					const std::string clip = obj.animName.empty() ? "IDLE" : obj.animName;
					scene.SetAnimation(g->GetID(), clip);
				}
			}
			else {
				g = scene.SpawnStaticSprite(obj.texture, { obj.x, obj.y, obj.z }, { obj.w, obj.h }, layerName);
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
			g->EnableShadow(obj.shadow);

			// Collider data
			if (!obj.hasCollider) {
				g->SetColliderSize({ 0.f, 0.f });
				g->SetColliderOffset({ 0.f, 0.f });
			}
			else {
				g->SetColliderSize({ obj.colWidth, obj.colHeight });
				g->SetColliderOffset({ obj.colOffsetX, obj.colOffsetY });

				// Fallback only for legacy/bad data (when collider is supposed to exist)
				if (obj.colWidth <= 0.f || obj.colHeight <= 0.f) {
					const glm::vec3 s = g->GetScaleGLM();
					g->SetColliderSize({ s.x, s.y });
					g->SetColliderOffset({ 0.f, 0.f });
				}
			}

			// Track texture path
			scene.SetObjectTexturePath(g->GetID(), obj.texture);

			// Tag-based special IDs and velocity
			scene.SetObjectTag(g->GetID(), obj.tag);
			scene.ApplyTagRules(g->GetID(), obj.tag, obj.speedX, obj.speedY);

			// Store transform and defaults
			scene.SetTransformFromLevel(g->GetID(), { obj.x, obj.y, obj.z }, { obj.w, obj.h, 1.0f }, obj.rotation);

			Scene::Defaults defs{};
			defs.pos = { obj.x, obj.y, obj.z };
			defs.size = { obj.w, obj.h };
			defs.rot = obj.rotation;
			defs.colSize = { obj.colWidth, obj.colHeight };
			defs.colOff = { obj.colOffsetX, obj.colOffsetY };
			defs.vel = { obj.speedX, obj.speedY };
			defs.texture = obj.texture;
			defs.tag = obj.tag;
			defs.layer = obj.layer;

			// Approach offset
			defs.approachOffset = { obj.approachOffsetX, obj.approachOffsetY };

			// Audio bindings
			defs.audioOnSpawn = obj.audioOnSpawn;
			defs.audioOnInteract = obj.audioOnInteract;
			defs.audioOnDestroy = obj.audioOnDestroy;
			defs.audioOnProcessing = obj.audioOnProcessing;
			defs.audioLoop = obj.audioLoop;

			defs.visible = obj.visible;

			scene.SetDefaults(g->GetID(), defs);
			scene.SetObjectVisible(g->GetID(), obj.visible);
			scene.AttachLogicForTag(g->GetID(), obj.tag);
			scene.ClampToWalkArea(g);

			if (!obj.prefabPath.empty()) {
				LELINKS::PrefabLinkByID[g->GetID()] = obj.prefabPath;
			}

			// Play spawn audio if configured (only during simulation/play mode)
			if (scene.IsSimulationActive() && !obj.audioOnSpawn.empty()) {
				scene.PlaySpawnAudio(g->GetID());
			}
		}
	}

	// Scene to LevelData snapshot
	void SyncSceneToLevel(Scene& scene, LevelData& levelOut) {
		levelOut.objects.clear();

		std::vector<GameObject*> list = scene.GetAllObjectsRaw();

		for (GameObject* g : list) {
			if (!g) {
				continue;
			}

			const int id = g->GetID();

			LevelObject out{};
			out.texture = scene.GetObjectTexturePath(id);
			out.animated = scene.HasAnimations(id);
			out.animName = scene.GetCurrentAnimationName(id);

			// Layer — use the layering system, fall back to "1"
			out.layer = scene.GetObjectLayer(id);
			if (out.layer.empty()) {
				out.layer = "1";
			}

			// Transform
			glm::vec3 pos = g->GetPositionGLM();
			glm::vec3 size = g->GetScaleGLM();
			float rotDeg = glm::degrees(g->GetRotationAngleZ());

			out.x = pos.x;
			out.y = pos.y;
			out.z = pos.z;
			out.w = size.x;
			out.h = size.y;
			out.rotation = rotDeg;

			// Collider
			const auto csz = g->GetColliderSize();
			const auto cof = g->GetColliderOffset();
			out.colWidth = csz.x;
			out.colHeight = csz.y;
			out.hasCollider = (csz.x > 0.f && csz.y > 0.f);
			out.colOffsetX = cof.x;
			out.colOffsetY = cof.y;

			// Use the value stored in Defaults as the single source of truth
			Scene::Defaults defs = scene.GetDefaults(id);

			// Tag comes from scene
			std::string tag = scene.GetObjectTag(id);
			if (tag.empty()) {
				tag = defs.tag; // fallback if not registered
			}

			out.tag = tag;

			out.approachOffsetX = defs.approachOffset.x;
			out.approachOffsetY = defs.approachOffset.y;

			// Audio bindings from defaults
			out.audioOnSpawn = defs.audioOnSpawn;
			out.audioOnInteract = defs.audioOnInteract;
			out.audioOnDestroy = defs.audioOnDestroy;
			out.audioOnProcessing = defs.audioOnProcessing;
			out.audioLoop = defs.audioLoop;

			out.shadow = g->HasShadow();
			out.visible = scene.IsObjectVisible(id);

			// Velocity
			const glm::vec2 v = scene.GetNPCVelocity(id);
			out.speedX = v.x;
			out.speedY = v.y;

			auto prefabIt = LELINKS::PrefabLinkByID.find(id);
			if (prefabIt != LELINKS::PrefabLinkByID.end()) {
				out.prefabPath = prefabIt->second;
			}

			levelOut.objects.push_back(out);
		}
	}

	// Swap transform + default-pos between two objects, if both exist.
	void SwapObjectPositions(Scene& scene, int aID, int bID) {
		if (aID == -1 || bID == -1 || aID == bID)
			return;

		GameObject* a = scene.GetGameObjectByID(aID);
		GameObject* b = scene.GetGameObjectByID(bID);
		if (!a || !b)
			return;

		// Swap scene transforms
		glm::vec3 posA = a->GetPositionGLM();
		glm::vec3 posB = b->GetPositionGLM();

		a->SetPosition(posB);
		b->SetPosition(posA);

		// Also swap default positions so saving/reloading stays consistent
		Scene::Defaults defA = scene.GetDefaults(aID);
		Scene::Defaults defB = scene.GetDefaults(bID);
		std::swap(defA.pos, defB.pos);
		scene.SetDefaults(aID, defA);
		scene.SetDefaults(bID, defB);
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
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 88.0f);
		ImGui::InputText("##NewLayerName", newLayerBuf, IM_ARRAYSIZE(newLayerBuf));
		ImGui::SameLine();

		if (ImGui::Button("Add", ImVec2(56.0f, 0.0f)) && newLayerBuf[0] != '\0') {
			scene.AddLayer(newLayerBuf);
			newLayerBuf[0] = '\0';
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Add layer");
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Existing layers:");

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

		std::sort(sorted.begin(), sorted.end(),
			[](const auto& lhs, const auto& rhs) {
				return lhs.first < rhs.first;
			});

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
			ImGui::SameLine();
			ImGui::TextDisabled("(%d)", count);
			ImGui::SameLine();
			if (ImGui::Checkbox("V", &visible)) {
				layer->SetVisible(visible);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Layer visibility");
			}

			ImGui::SameLine();

			// Collisions checkbox
			if (ImGui::Checkbox("C", &collidable)) {
				layer->SetCollidable(collidable);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Layer collisions");
			}

			if (selectedObjectId != -1) {
				if (ImGui::SmallButton("Assign")) {
					scene.AssignObjectToLayer(selectedObjectId, layerName);
				}
			}
			else {
				ImGui::BeginDisabled();
				ImGui::SmallButton("Assign");
				ImGui::EndDisabled();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Assign selected object to this layer");
			}

			ImGui::SameLine();
			if (layerName == "1") {
				ImGui::BeginDisabled();
				ImGui::SmallButton("Del");
				ImGui::EndDisabled();
			}
			else if (ImGui::SmallButton("Del")) {
				for (int objID : layer->GetObjects()) {
					scene.AssignObjectToLayer(objID, "1");
				}
				scene.RemoveLayer(layerName);
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Delete layer (moves objects to layer 1)");
			}

			ImGui::Separator();
			ImGui::PopID();
		}

		if (selectedObjectId == -1) {
			ImGui::TextDisabled("Select an object to enable Assign.");
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

		// Force "no default level" once per run
		static bool sClearedDefaultOnce = false;
		if (!sClearedDefaultOnce) {
			editor.levelPath.clear();
			sClearedDefaultOnce = true;
		}

		if (!ImGui::Begin("Level###LE_Level")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Level Management");

		static char levelPathBuf[256] = { 0 };

		// Level path row (ALWAYS use repo_root/levels, not ../levels)
		static const std::string sLevelsDir = (FindRepoRoot() / "levels").generic_string();

		// Default to repo_root/levels/kitchen01.json
		if (editor.levelPath.empty()) {
			// No default: designer must pick from dropdown
			levelPathBuf[0] = '\0';
		}
		else if (levelPathBuf[0] == '\0') {
			// If editor.levelPath was set elsewhere, sync display buffer once
			std::string stem = fs::path(editor.levelPath).stem().string();
			std::snprintf(levelPathBuf, sizeof(levelPathBuf), "%s", stem.c_str());
		}

		// List files from repo_root/levels (can be absolute paths depending on ListJsonFiles)
		static std::vector<std::string> sLevelFiles = ListJsonFiles(sLevelsDir);

		ImGui::TextUnformatted("Level path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##levels")) {
			sLevelFiles = ListJsonFiles(sLevelsDir);
		}

		// Combo preview label should show only the current name
		const std::string currentName = fs::path(editor.levelPath).stem().string();

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		const char* comboPreview = (levelPathBuf[0] == '\0') ? "<select level>" : levelPathBuf;
		if (ImGui::BeginCombo("##LevelCombo", comboPreview)) {
			for (size_t i = 0; i < sLevelFiles.size(); ++i) {
				const fs::path p(sLevelFiles[i]);
				const std::string displayName = p.stem().string(); // e.g. "kitchen01"

				const bool isSelected = (displayName == currentName);
				if (ImGui::Selectable(displayName.c_str(), isSelected)) {
					// Always set absolute path internally
					editor.levelPath = (fs::path(sLevelsDir) / (displayName + ".json")).generic_string();
					std::snprintf(levelPathBuf, sizeof(levelPathBuf), "%s", displayName.c_str());
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::InputText("##LevelPathEdit", levelPathBuf, IM_ARRAYSIZE(levelPathBuf))) {
			// Accept "kitchen01" or "kitchen01.json" or even a path; normalize to just stem
			std::string typed = levelPathBuf;
			std::string stem = fs::path(typed).stem().string();

			// Update buffer to normalized name
			std::snprintf(levelPathBuf, sizeof(levelPathBuf), "%s", stem.c_str());

			// Always set absolute path internally
			editor.levelPath = (fs::path(sLevelsDir) / (stem + ".json")).generic_string();
		}

		// Ctrl+Z keyboard shortcut for Undo (same as button)
		ImGuiIO& io = ImGui::GetIO();
		if (!editor.IsPlaying() &&
			!io.WantCaptureKeyboard &&
			(io.KeyCtrl || io.KeySuper) &&
			!io.KeyShift &&
			ImGui::IsKeyPressed(ImGuiKey_Z)) {
			if (PerformUndo(editor, scene)) {
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		// Ctrl+Y or Ctrl+Shift+Z keyboard shortcuts for Redo
		if (!editor.IsPlaying() &&
			!io.WantCaptureKeyboard &&
			(io.KeyCtrl || io.KeySuper) &&
			(ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) {
			if (PerformRedo(editor, scene)) {
				selectedIndex = -1;
				selectedObjectId = -1;
			}
		}

		// Level actions in a compact grid to reduce horizontal crowding
		if (ImGui::BeginTable("##LevelActionsGrid", 4, ImGuiTableFlags_SizingStretchSame)) {
			// Row 1
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Load Level", ImVec2(-FLT_MIN, 0.0f))) {
				LevelData& work = editor.MutableLevel();
				if (LevelSerializer::Load(editor.levelPath, work)) {
					scene.ClearAll();
					LEPANELFONTS::ClearTextObjects();  // Clear text objects before loading
					SyncLevelToScene(work, scene);
					SyncTextObjectsToEditor(work);     // Load text objects
					scene.RebuildColliders();
					scene.SetSimulationActive(false);
					scene.ResetResizeBaseline();

					if (!work.background.empty()) {
						scene.SetSceneBackground(work.background);
					}

					editor.SetPlaying(false);
					selectedIndex = -1;
					selectedObjectId = -1;
					ClearUndoHistory();
				}
			}

			ImGui::TableSetColumnIndex(1);
			if (ImGui::Button("New Scene", ImVec2(-FLT_MIN, 0.0f))) {
				scene.StopAllObjectAudio();
				scene.SetSimulationActive(false);
				scene.ClearAll();
				scene.RebuildColliders();
				scene.ResetResizeBaseline();

				editor.SetPlaying(false);
				LEPANELFONTS::ClearTextObjects();

				LevelData& fresh = editor.MutableLevel();
				fresh.objects.clear();
				fresh.textObjects.clear();
				fresh.background.clear();
				ClearUndoHistory();
			}

			ImGui::TableSetColumnIndex(2);
			if (ImGui::Button("Save Level", ImVec2(-FLT_MIN, 0.0f))) {
				LevelData& dst = editor.MutableLevel();
				SyncSceneToLevel(scene, dst);
				SyncTextObjectsToLevel(dst);  // Save text objects

				std::cout << "[LevelPanel] Saving level to: " << editor.levelPath << std::endl;
				std::cout << "[LevelPanel] Game objects: " << dst.objects.size() << std::endl;
				std::cout << "[LevelPanel] Text objects: " << dst.textObjects.size() << std::endl;

				if (LevelSerializer::Save(editor.levelPath, dst)) {
					std::cout << "[LevelPanel] Level saved successfully!" << std::endl;
				}
				else {
					std::cerr << "[LevelPanel] ERROR: Failed to save level!" << std::endl;
				}
			}

			ImGui::TableSetColumnIndex(3);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Undo", ImVec2(-FLT_MIN, 0.0f))) {
				if (PerformUndo(editor, scene)) {
					selectedIndex = -1;
					selectedObjectId = -1;
				}
			}

			ImGui::EndDisabled();

			// Row 2
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Redo", ImVec2(-FLT_MIN, 0.0f))) {
				if (PerformRedo(editor, scene)) {
					selectedIndex = -1;
					selectedObjectId = -1;
				}
			}

			ImGui::EndDisabled();

			ImGui::TableSetColumnIndex(1);
			ImGui::BeginDisabled(editor.IsPlaying());
			if (ImGui::Button("Play", ImVec2(-FLT_MIN, 0.0f))) {
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

			ImGui::EndDisabled();

			ImGui::TableSetColumnIndex(2);
			ImGui::BeginDisabled(!editor.IsPlaying());
			if (ImGui::Button("Stop", ImVec2(-FLT_MIN, 0.0f))) {
				// Stop all object-bound audio before clearing the scene
				scene.StopAllObjectAudio();
				scene.SetSimulationActive(false);

				scene.ClearAll();
				SyncLevelToScene(editor.MutablePlaySnapshot(), scene);
				scene.RebuildColliders();
				editor.SetPlaying(false);
			}

			ImGui::EndDisabled();

			ImGui::TableSetColumnIndex(3);
			const bool isSimActive = scene.IsSimulationActive();
			const char* pauseLabel = isSimActive ? "Pause" : "Resume";
			ImGui::BeginDisabled(!editor.IsPlaying());
			if (ImGui::Button(pauseLabel, ImVec2(-FLT_MIN, 0.0f))) {
				scene.SetSimulationActive(!isSimActive);
			}

			ImGui::EndDisabled();

			ImGui::EndTable();
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
			// Game Objects
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
					? ("ID " + std::to_string(gid) + " [Layer: " + layer + "]")
					: (niceName + " (ID " + std::to_string(gid) + ") [Layer: " + layer + "]");

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
						// Deselect any text object when selecting a game object
						LEPANELFONTS::SetSelectedTextIndex(-1);
					}
				}

				ImGui::PopID();
			}

			// Text Objects in Hierarchy (inside the same listbox)
			{
				const auto& textObjs = LEPANELFONTS::GetTextObjects();
				int selectedTextIdx = LEPANELFONTS::GetSelectedTextIndex();

				for (size_t i = 0; i < textObjs.size(); ++i) {
					const auto& t = textObjs[i];

					// Check if layer is visible
					Layer* layer = scene.GetLayer(t.layer);
					if (layer && !layer->IsVisible()) {
						continue;
					}

					std::string lbl = "[Text] " + t.name + " (" + t.fontName + ") [Layer: " + t.layer + "]";

					ImGui::PushID(static_cast<int>(i) + 100000);
					bool isSelected = (selectedObjectId == -1 && selectedTextIdx == static_cast<int>(i));

					if (editor.IsPlaying()) {
						ImGui::Selectable(lbl.c_str(), isSelected, ImGuiSelectableFlags_Disabled);
					}
					else {
						if (ImGui::Selectable(lbl.c_str(), isSelected)) {
							selectedIndex = -1;
							selectedObjectId = -1;
							LEPANELFONTS::SetSelectedTextIndex(static_cast<int>(i));
						}
					}

					ImGui::PopID();
				}
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

			proto.colWidth = proto.w;
			proto.colHeight = proto.h;
			proto.colOffsetX = 0.f;
			proto.colOffsetY = 0.f;

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
				scene.RebuildColliders();
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

			Scene::Defaults defaults = scene.GetDefaults(id);

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
			{
				std::string tag = scene.GetObjectTag(id);
				if (tag.empty()) tag = defaults.tag;
				std::snprintf(tagBuf, sizeof(tagBuf), "%s", tag.c_str());
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

			bool colliderEnabled = (colliderSize.x > 0.f && colliderSize.y > 0.f);

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

				if (colliderSize.x <= 0.f || colliderSize.y <= 0.f) {
					colliderSize.x = size.x;
					colliderSize.y = size.y;
					obj->SetColliderSize({ colliderSize.x, colliderSize.y });
					obj->SetColliderOffset({ 0.f, 0.f });
					scene.RebuildColliders();
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

						// Keep defaults in sync so resets / hierarchy labels behave correctly
						defaults.layer = name;
						scene.SetDefaults(id, defaults);
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

				if (colliderEnabled) {
					colliderSize.x = size.x;
					colliderSize.y = size.y;
					obj->SetColliderSize({ colliderSize.x, colliderSize.y });
					scene.RebuildColliders();
				}

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
									PushUndoSnapshot(editor, scene);
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
			DragFloatWithReset("##rot", &rotationDeg, defaults.rot, 0.8f, [&](bool) {
				// Clamp to [0, 360) before applying so it never stores huge angles
				rotationDeg = std::fmod(rotationDeg, 360.0f);
				if (rotationDeg < 0.0f) {
					rotationDeg += 360.0f;
				}

				obj->SetRotation(glm::radians(rotationDeg), { 0, 0, 1 });
				scene.SetTransformFromLevel(id, position, { size.x, size.y, 1.0f }, rotationDeg);
				});
			ImGui::NextColumn();

			// Collider
			ImGui::Text("Collider");
			ImGui::NextColumn();
			FullWidthNext();

			if (ImGui::Checkbox("Enable Collider", &colliderEnabled)) {
				PushUndoSnapshot(editor, scene);

				if (!colliderEnabled) {
					colliderSize = { 0.f, 0.f };
					colliderOff = { 0.f, 0.f };
					obj->SetColliderSize({ 0.f, 0.f });
					obj->SetColliderOffset({ 0.f, 0.f });
				}
				else {
					// default collider when adding
					colliderSize = { size.x, size.y };
					colliderOff = { 0.f, 0.f };
					obj->SetColliderSize({ colliderSize.x, colliderSize.y });
					obj->SetColliderOffset({ 0.f, 0.f });
				}

				scene.RebuildColliders();
			}

			if (colliderEnabled) {
				float avail = ImGui::GetContentRegionAvail().x;
				float gap = ImGui::GetStyle().ItemInnerSpacing.x;
				float fieldW = (avail - gap) * 0.5f;

				// Size
				ImGui::TextUnformatted("Size");
				// row of 2 fields (x, y)
				ImGui::PushItemWidth(fieldW);
				bool sizeChangedX = ImGui::DragFloat("x##col_size_x", &colliderSize.x, 1.0f, 0.0f, 99999.0f);
				bool sizeActivatedX = ImGui::IsItemActivated();
				ImGui::SameLine(0.0f, gap);
				bool sizeChangedY = ImGui::DragFloat("y##col_size_y", &colliderSize.y, 1.0f, 0.0f, 99999.0f);
				bool sizeActivatedY = ImGui::IsItemActivated();
				ImGui::PopItemWidth();

				if (sizeActivatedX || sizeActivatedY) {
					PushUndoSnapshot(editor, scene);
				}

				if (sizeChangedX || sizeChangedY) {
					obj->SetColliderSize({ colliderSize.x, colliderSize.y });
					scene.RebuildColliders();
				}

				ImGui::Spacing();

				// Offset
				ImGui::TextUnformatted("Offset");
				ImGui::PushItemWidth(fieldW);
				bool offChangedX = ImGui::DragFloat("x##col_off_x", &colliderOff.x, 1.0f, -99999.0f, 99999.0f);
				bool offActivatedX = ImGui::IsItemActivated();
				ImGui::SameLine(0.0f, gap);
				bool offChangedY = ImGui::DragFloat("y##col_off_y", &colliderOff.y, 1.0f, -99999.0f, 99999.0f);
				bool offActivatedY = ImGui::IsItemActivated();
				ImGui::PopItemWidth();

				if (offActivatedX || offActivatedY) {
					PushUndoSnapshot(editor, scene);
				}

				if (offChangedX || offChangedY) {
					obj->SetColliderOffset({ colliderOff.x, colliderOff.y });
					scene.RebuildColliders();
				}
			}
			else {
				ImGui::TextDisabled("Collider disabled");
			}

			ImGui::NextColumn();

			ImGui::Columns(1);

			// ========== Audio Bindings Section ==========
			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Audio Bindings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent(8.0f);

				// Get available audio assets from catalog
				const auto& audioAssets = Audio::AudioCatalog::GetAllAssets();

				// Build list of audio names for combo boxes
				std::vector<const char*> audioNames;
				audioNames.push_back("(None)");  // First option to clear binding
				for (const auto& asset : audioAssets) {
					audioNames.push_back(asset.name.c_str());
				}

				// Helper lambda to draw audio slot with combo and drag-drop
				auto DrawAudioSlot = [&](const char* label, std::string& audioBinding, const char* tooltipText) {
					ImGui::Text("%s", label);
					ImGui::SameLine(120.0f);

					// Find current selection index
					int currentIdx = 0;
					for (size_t i = 1; i < audioNames.size(); ++i) {
						if (audioBinding == audioNames[i]) {
							currentIdx = static_cast<int>(i);
							break;
						}
					}

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
					std::string comboId = std::string("##") + label;
					if (ImGui::BeginCombo(comboId.c_str(), audioNames[currentIdx])) {
						for (size_t i = 0; i < audioNames.size(); ++i) {
							bool isSelected = (currentIdx == static_cast<int>(i));
							if (ImGui::Selectable(audioNames[i], isSelected)) {
								if (ImGui::IsItemActivated()) {
									PushUndoSnapshot(editor, scene);
								}
								audioBinding = (i == 0) ? "" : audioNames[i];
								if (label == std::string("On Spawn")) defaults.audioOnSpawn = audioBinding;
								else if (label == std::string("On Interact")) defaults.audioOnInteract = audioBinding;
								else if (label == std::string("On Destroy")) defaults.audioOnDestroy = audioBinding;
								else if (label == std::string("On Processing")) defaults.audioOnProcessing = audioBinding;
								scene.SetDefaults(id, defaults);
							}
							if (isSelected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					// Drag-drop target for audio assets
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AUDIO_ASSET")) {
							const char* droppedName = static_cast<const char*>(payload->Data);
							if (droppedName) {
								PushUndoSnapshot(editor, scene);
								audioBinding = droppedName;
								if (label == std::string("On Spawn")) defaults.audioOnSpawn = audioBinding;
								else if (label == std::string("On Interact")) defaults.audioOnInteract = audioBinding;
								else if (label == std::string("On Destroy")) defaults.audioOnDestroy = audioBinding;
								else if (label == std::string("On Processing")) defaults.audioOnProcessing = audioBinding;
								scene.SetDefaults(id, defaults);
							}
						}
						ImGui::EndDragDropTarget();
					}

					// Clear button
					ImGui::SameLine();
					std::string clearBtnId = std::string("X##clear_") + label;
					if (ImGui::Button(clearBtnId.c_str(), ImVec2(20, 0))) {
						PushUndoSnapshot(editor, scene);
						audioBinding = "";
						if (label == std::string("On Spawn")) defaults.audioOnSpawn = "";
						else if (label == std::string("On Interact")) defaults.audioOnInteract = "";
						else if (label == std::string("On Destroy")) defaults.audioOnDestroy = "";
						else if (label == std::string("On Processing")) defaults.audioOnProcessing = "";
						scene.SetDefaults(id, defaults);
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Clear audio binding");
					}

					// Preview button
					if (!audioBinding.empty()) {
						ImGui::SameLine();
						std::string previewBtnId = std::string(">##preview_") + label;
						if (ImGui::Button(previewBtnId.c_str(), ImVec2(20, 0))) {
							// Play preview through AudioCatalog
							// Note: This would need AudioManager access, simplified here
						}
						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Preview audio");
						}
					}

					if (ImGui::IsItemHovered() && tooltipText) {
						ImGui::SetTooltip("%s", tooltipText);
					}
					};

				// Get current audio bindings from defaults
				std::string audioOnSpawn = defaults.audioOnSpawn;
				std::string audioOnInteract = defaults.audioOnInteract;
				std::string audioOnDestroy = defaults.audioOnDestroy;
				std::string audioOnProcessing = defaults.audioOnProcessing;

				DrawAudioSlot("On Spawn", audioOnSpawn, "Audio played when object spawns/loads");
				DrawAudioSlot("On Interact", audioOnInteract, "Audio played when player interacts");
				DrawAudioSlot("On Destroy", audioOnDestroy, "Audio played when object is destroyed");
				DrawAudioSlot("On Processing", audioOnProcessing, "Audio played while work table is processing (loops)");

				// Audio loop checkbox for spawn audio
				ImGui::Spacing();
				bool audioLoop = defaults.audioLoop;
				if (ImGui::Checkbox("Loop Spawn Audio", &audioLoop)) {
					PushUndoSnapshot(editor, scene);
					defaults.audioLoop = audioLoop;
					scene.SetDefaults(id, defaults);
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("If checked, the 'On Spawn' audio will loop continuously");
				}

				ImGui::Spacing();
				ImGui::TextDisabled("Drag audio from Audio Control panel to slots above");

				ImGui::Unindent(8.0f);
			}
			// ========== End Audio Bindings Section ==========

			std::string newTag = tagBuf;

			// persist tag in both defaults + scene registry
			defaults.tag = newTag;
			scene.SetDefaults(id, defaults);
			scene.SetObjectTag(id, newTag);

			// centralizes special IDs + velocity rules
			scene.ApplyTagRules(id, newTag, defaults.vel.x, defaults.vel.y);

			// logic attach remains centralized in scene
			scene.AttachLogicForTag(id, newTag);

			if (editor.IsPlaying()) {
				ImGui::EndDisabled();
			}
		}
		// Text Object Inspector - when a text object is selected
		else {
			int selectedTextIdx = LEPANELFONTS::GetSelectedTextIndex();
			auto& textObjs = LEPANELFONTS::GetMutableTextObjects();

			if (selectedTextIdx >= 0 && selectedTextIdx < static_cast<int>(textObjs.size())) {
				if (editor.IsPlaying()) {
					ImGui::BeginDisabled();
				}

				LEPANELFONTS::TextObjectData& textObj = textObjs[selectedTextIdx];

				ImGui::SeparatorText("Text Object Properties");
				ImGui::TextDisabled("Selected Text: %s", textObj.name.c_str());

				ImGui::Columns(2, nullptr, false);
				ImGuiStyle& style = ImGui::GetStyle();

				float longestLabel = 0.0f;
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Name").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Font").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Text").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Layer").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Position X").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Position Y").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Scale").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Rotation").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Rotation Mode").x);
				longestLabel = ImMax(longestLabel, ImGui::CalcTextSize("Color").x);
				float labelColWidth = longestLabel + style.ItemInnerSpacing.x * 2.0f + 12.0f;
				ImGui::SetColumnWidth(0, labelColWidth);

				auto FullWidthNext = []() {
					ImGui::SetNextItemWidth(-FLT_MIN);
					};

				// Name
				ImGui::TextUnformatted("Name");
				ImGui::NextColumn();
				FullWidthNext();
				char nameBuf[64];
				std::snprintf(nameBuf, sizeof(nameBuf), "%s", textObj.name.c_str());
				if (ImGui::InputText("##TextName", nameBuf, IM_ARRAYSIZE(nameBuf))) {
					textObj.name = nameBuf;
				}

				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}

				ImGui::NextColumn();

				// Font selection
				ImGui::TextUnformatted("Font");
				ImGui::NextColumn();
				FullWidthNext();
				{
					// Get list of loaded fonts from LEPANELFONTS
					const auto& fontNames = LEPANELFONTS::GetLoadedFontNames();
					if (ImGui::BeginCombo("##TextFont", textObj.fontName.c_str())) {
						for (const auto& fontName : fontNames) {
							const bool isSelected = (textObj.fontName == fontName);
							if (ImGui::Selectable(fontName.c_str(), isSelected)) {
								PushUndoSnapshot(editor, scene);
								textObj.fontName = fontName;
							}
							if (isSelected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}
				ImGui::NextColumn();

				// Text content
				ImGui::TextUnformatted("Text");
				ImGui::NextColumn();
				FullWidthNext();
				char textBuf[256];
				std::snprintf(textBuf, sizeof(textBuf), "%s", textObj.text.c_str());
				if (ImGui::InputText("##TextContent", textBuf, IM_ARRAYSIZE(textBuf))) {
					textObj.text = textBuf;
				}

				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}

				ImGui::NextColumn();

				// Layer selection
				ImGui::TextUnformatted("Layer");
				ImGui::NextColumn();
				FullWidthNext();
				{
					std::vector<std::string> layerNames;
					layerNames.push_back("1");

					const auto& allLayers = scene.GetAllLayers();
					for (const auto& pair : allLayers) {
						const std::string& name = pair.first;
						if (name.empty()) continue;
						if (std::find(layerNames.begin(), layerNames.end(), name) == layerNames.end()) {
							layerNames.push_back(name);
						}
					}
					std::sort(layerNames.begin(), layerNames.end());

					if (ImGui::BeginCombo("##TextLayer", textObj.layer.c_str())) {
						for (const std::string& name : layerNames) {
							bool isSelected = (textObj.layer == name);
							if (ImGui::Selectable(name.c_str(), isSelected)) {
								PushUndoSnapshot(editor, scene);
								textObj.layer = name;
								scene.AddLayer(name);
							}
							if (isSelected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
				}
				ImGui::NextColumn();

				// Position X
				ImGui::TextUnformatted("Position X");
				ImGui::NextColumn();
				FullWidthNext();
				ImGui::DragFloat("##TextPosX", &textObj.x, 1.0f);
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}
				ImGui::NextColumn();

				// Position Y
				ImGui::TextUnformatted("Position Y");
				ImGui::NextColumn();
				FullWidthNext();
				ImGui::DragFloat("##TextPosY", &textObj.y, 1.0f);
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}
				ImGui::NextColumn();

				// Scale
				ImGui::TextUnformatted("Scale");
				ImGui::NextColumn();
				FullWidthNext();
				ImGui::DragFloat("##TextScale", &textObj.scale, 0.01f, 0.1f, 10.0f);
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}
				ImGui::NextColumn();

				// Rotation
				ImGui::TextUnformatted("Rotation");
				ImGui::NextColumn();
				FullWidthNext();
				ImGui::SliderFloat("##TextRotation", &textObj.rotation, 0.0f, 360.0f, "%.1f deg");
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}
				ImGui::NextColumn();

				// Rotation Mode
				ImGui::TextUnformatted("Rotation Mode");
				ImGui::NextColumn();
				FullWidthNext();
				const char* rotModeItems[] = { "Block (Normal)", "Per-Character (Curved)" };
				int currentMode = textObj.useBlockRotation ? 0 : 1;
				if (ImGui::Combo("##TextRotMode", &currentMode, rotModeItems, IM_ARRAYSIZE(rotModeItems))) {
					PushUndoSnapshot(editor, scene);
					textObj.useBlockRotation = (currentMode == 0);
				}
				ImGui::NextColumn();

				// Color
				ImGui::TextUnformatted("Color");
				ImGui::NextColumn();
				FullWidthNext();
				float color[4] = { textObj.colorR, textObj.colorG, textObj.colorB, textObj.colorA };
				if (ImGui::ColorEdit4("##TextColor", color)) {
					textObj.colorR = color[0];
					textObj.colorG = color[1];
					textObj.colorB = color[2];
					textObj.colorA = color[3];
				}
				if (ImGui::IsItemActivated()) {
					PushUndoSnapshot(editor, scene);
				}
				ImGui::NextColumn();

				ImGui::Columns(1);

				if (editor.IsPlaying()) {
					ImGui::EndDisabled();
				}
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
				const std::string dropped = droppedCStr ? std::string(droppedCStr) : std::string();

				LevelObject data{};
				if (LEFILEIO::LoadPrefabFromFile(dropped, data)) {
					// Snapshot BEFORE creating instance from prefab
					PushUndoSnapshot(editor, scene);

					std::string prefabLayer = data.layer.empty() ? "1" : data.layer;
					GameObject* g = scene.SpawnStaticSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						prefabLayer);

					if (g) {
						// link prefab to instance for propagation
						LELINKS::PrefabLinkByID[g->GetID()] = dropped;

						g->SetRotation(glm::radians(data.rotation), { 0, 0, 1 });

						if (data.colWidth > 0.f && data.colHeight > 0.f) {
							g->SetColliderSize({ data.colWidth,  data.colHeight });
							g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });
						}

						scene.SetObjectTexturePath(g->GetID(), data.texture);
						scene.SetTransformFromLevel(
							g->GetID(),
							{ data.x, data.y, data.z },
							{ data.w, data.h, 1.0f },
							data.rotation);
						scene.SetNPCVelocity(g->GetID(), data.speedX, data.speedY);
						scene.ClampToWalkArea(g);

						scene.RebuildColliders();
					}
				}
			}

			// Texture dropped to apply to selected
			if (const ImGuiPayload* tp = ImGui::AcceptDragDropPayload("ASSET_PATH")) {
				const char* droppedCStr = static_cast<const char*>(tp->Data);
				const std::string dropped = droppedCStr ? std::string(droppedCStr) : std::string();

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

							auto col = o->GetColliderSize();
							if (col.x <= 0.f || col.y <= 0.f) {
								glm::vec3 s = o->GetScaleGLM();
								o->SetColliderSize({ s.x, s.y });
								o->SetColliderOffset({ 0.f, 0.f });
								scene.RebuildColliders();
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