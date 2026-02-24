/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelPrefabs.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implementation of the Level Editor Prefabs panel.
					- Select/refresh prefab paths
					- Save selected scene object as a prefab (JSON)
					- Instantiate a new object from a prefab
					- Propagate prefab changes to all linked instances

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <unordered_map>

#ifdef _DEBUG
#include <imgui.h>
#endif

#include <string>
#include <vector>

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "LevelEditorPanelPrefabs.hpp"
#include "LevelEditorPrefabLinks.hpp"
#include "LevelSerializer.hpp"
#include "FilePaths.hpp"

namespace fs = std::filesystem;

using namespace LEFILEIO;
using namespace LELINKS;

namespace LEPANELPREFABS {
#ifdef _DEBUG
	static LevelObject BuildPrefabFromObject(Scene& scene, GameObject* g) {
		LevelObject out{};

		out.texture = scene.GetObjectTexturePath(g->GetID());

		const glm::vec3 p = g->GetPositionGLM();
		const glm::vec3 s = g->GetScaleGLM();

		out.x = p.x;
		out.y = p.y;
		out.z = p.z;
		out.w = s.x;
		out.h = s.y;

		out.rotation = glm::degrees(g->GetRotationAngleZ());

		const auto csz = g->GetColliderSize();
		const auto cof = g->GetColliderOffset();

		out.colWidth = csz.x;
		out.colHeight = csz.y;
		out.colOffsetX = cof.x;
		out.colOffsetY = cof.y;

		out.animated = scene.HasAnimations(g->GetID());
		out.layer = scene.GetObjectLayer(g->GetID());

		const glm::vec2 v = scene.GetNPCVelocity(g->GetID());
		out.speedX = v.x;
		out.speedY = v.y;

		return out;
	}

	// Ensure a .json extension for save paths
	static inline void EnsureJsonExt(std::string& path) {
		if (fs::path(path).extension().empty()) {
			path += ".json";
		}
	}

	static std::string NormalizePrefabPath(const std::string& path) {
		std::error_code ec;
		const fs::path raw(path);
		const fs::path canonical = fs::weakly_canonical(raw, ec);
		if (!ec) {
			return canonical.lexically_normal().generic_string();
		}

		return raw.lexically_normal().generic_string();
	}

	static bool IsSamePrefabPath(const std::string& lhs, const std::string& rhs) {
		if (lhs == rhs) {
			return true;
		}

		return NormalizePrefabPath(lhs) == NormalizePrefabPath(rhs);
	}

	static std::string ResolveLevelsDirectoryForPropagation() {
		std::error_code ec;

		fs::path probe = fs::current_path(ec);
		if (!ec) {
			for (int i = 0; i < 10; ++i) {
				const fs::path buildDir = probe / "build";
				const fs::path levelsDir = probe / "levels";

				if (fs::exists(buildDir, ec) && fs::is_directory(buildDir, ec) &&
					fs::exists(levelsDir, ec) && fs::is_directory(levelsDir, ec)) {
					return levelsDir.lexically_normal().generic_string();
				}

				if (!probe.has_parent_path()) {
					break;
				}

				probe = probe.parent_path();
			}
		}

		return FilePaths::Dirs::LEVELS_EDITOR;
	}

	static int PropagatePrefabToAllLevelFiles(const std::string& prefabPath, const LevelObject& updatedPrefab) {
		int totalObjectsUpdated = 0;
		const std::vector<std::string> levelFiles = ListJsonFiles(ResolveLevelsDirectoryForPropagation());

		for (const auto& levelPath : levelFiles) {
			LevelData levelData{};
			if (!LevelSerializer::Load(levelPath, levelData)) {
				continue;
			}

			bool dirty = false;

			for (auto& obj : levelData.objects) {
				if (!obj.prefabPath.empty() && IsSamePrefabPath(obj.prefabPath, prefabPath)) {
					const float keepX = obj.x;
					const float keepY = obj.y;
					const float keepZ = obj.z;

					obj = updatedPrefab;
					obj.x = keepX;
					obj.y = keepY;
					obj.z = keepZ;
					obj.prefabPath = prefabPath;

					dirty = true;
					++totalObjectsUpdated;
				}
			}

			if (dirty) {
				LevelSerializer::Save(levelPath, levelData);
			}
		}

		return totalObjectsUpdated;
	}

	// Draw the Prefabs docked window
	void DrawPrefabsPanel(LevelEditor& editor, Scene& scene, int& selectedObjectId) {
		ImGui::SetNextWindowDockID(
			GraphicsEngine::Instance().GetMainDockspaceID(),
			ImGuiCond_FirstUseEver
		);


		if (!ImGui::Begin("Prefabs###LE_Prefabs")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Prefabs / Archetypes");

		// Prefab path row (combo + input + refresh)
		static char prefabPathBuf[256] = {};
		static bool prefabPathInitialized = false;
		if (!prefabPathInitialized) {
			std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%smy_goat.json", FilePaths::Dirs::PREFABS_EDITOR);
			prefabPathInitialized = true;
		}
		static std::vector<std::string> sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);

		// Cache for prefab thumbnails (keyed by prefab JSON path)
		static std::unordered_map<std::string, Texture*> sPrefabPreviewCache;

		ImGui::TextUnformatted("Prefab path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##pf")) {
			sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Prefab...")) {
			const std::string picked =
				OpenFileDialog("JSON files\0*.json\0All files\0*.*\0");

			if (!picked.empty()) {
				const std::string targetDir = FilePaths::Dirs::PREFABS_EDITOR;

				const std::string projPath = CopyFileIntoProjectUnique(picked, targetDir);

				if (!projPath.empty()) {
					// Rebuild list in this panel
					sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);

					// Optional: auto-select the imported prefab in the combo
					fs::path filename = fs::path(projPath).filename();
					std::string displayPath = std::string(FilePaths::Dirs::PREFABS_EDITOR) + filename.string();
					std::snprintf(prefabPathBuf,
						sizeof(prefabPathBuf),
						"%s",
						displayPath.c_str());
				}
			}
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##PrefabCombo", prefabPathBuf)) {
			for (size_t i = 0; i < sPrefabs.size(); ++i) {
				bool selected = (sPrefabs[i] == prefabPathBuf);
				if (ImGui::Selectable(sPrefabs[i].c_str(), selected)) {
					std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", sPrefabs[i].c_str());
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##PrefabPathEdit", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

		std::string prefabPath = prefabPathBuf;
		EnsureJsonExt(prefabPath);
		const bool prefabExists = fs::exists(prefabPath);

		// Prefab list
		ImGui::Spacing();
		ImGui::SeparatorText("Prefab Library");

		if (ImGui::Button("Refresh##pf_list")) {
			sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);
			sPrefabPreviewCache.clear();
		}

		// Scrollable area for prefab thumbnails + paths
		ImGui::BeginChild("##PrefabList", ImVec2(0, 200.0f), true);

		bool refreshPrefabs = false;
		const float iconSize = 32.0f;

		for (const auto& path : sPrefabs) {
			ImGui::PushID(path.c_str());

			// Fetch or build a thumbnail for this prefab
			Texture* previewTex = nullptr;
			auto it = sPrefabPreviewCache.find(path);
			if (it != sPrefabPreviewCache.end()) {
				previewTex = it->second;
			}
			else {
				LevelObject data{};
				if (LoadPrefabFromFile(path, data) && !data.texture.empty()) {
					previewTex = LoadTextureBypassingCache(data.texture);
				}

				// Cache even nullptr so we don't keep trying failed loads
				sPrefabPreviewCache[path] = previewTex;
			}

			// Draw thumbnail
			if (previewTex) {
				ImTextureID texID = (ImTextureID)(intptr_t)previewTex->GetID();
				ImGui::Image(texID,
					ImVec2(iconSize, iconSize),
					ImVec2(0, 1),
					ImVec2(1, 0));
				ImGui::SameLine();
			}

			// Highlight currently selected prefab (the one in prefabPathBuf)
			bool isSelected = (std::strcmp(prefabPathBuf, path.c_str()) == 0);
			if (ImGui::Selectable(path.c_str(), isSelected,
				0, ImVec2(0.0f, iconSize))) {
				// Clicking on list item updates the active prefab path
				std::snprintf(prefabPathBuf,
					sizeof(prefabPathBuf),
					"%s", path.c_str());
			}

			// Drag source: other panels can accept "PREFAB_PATH"
			if (ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload("PREFAB_PATH",
					path.c_str(),
					path.size() + 1);
				ImGui::TextUnformatted("Prefab");
				ImGui::TextWrapped("%s", path.c_str());
				ImGui::EndDragDropSource();
			}

			// Right-click: soft delete
			if (ImGui::BeginPopupContextItem(
				(std::string("ctx_prefab##") + path).c_str())) {
				if (ImGui::MenuItem("Delete")) {
					if (MoveToTrash(path)) {
						refreshPrefabs = true;
						sPrefabPreviewCache.erase(path);
					}
				}
				ImGui::EndPopup();
			}

			ImGui::PopID();
		}

		if (refreshPrefabs) {
			sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);
			sPrefabPreviewCache.clear();
		}

		ImGui::EndChild();

		// Existing spacing + separator
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Save selected as prefab (disabled during Play)
		if (editor.IsPlaying()) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Save selected as prefab")) {
			if (selectedObjectId >= 0) {
				// Find the selected GameObject
				GameObject* gSel = scene.GetGameObjectByID(selectedObjectId);
				if (gSel) {
					LevelObject out{};

					// Texture path
					out.texture = scene.GetObjectTexturePath(selectedObjectId);

					// Transform (position/scale) – store rotation in DEGREES for JSON
					const glm::vec3 p = gSel->GetPositionGLM();
					const glm::vec3 s = gSel->GetScaleGLM();

					out.x = p.x; out.y = p.y; out.z = p.z;
					out.w = s.x; out.h = s.y;

					out.rotation = glm::degrees(gSel->GetRotationAngleZ());

					// Collider
					const auto csz = gSel->GetColliderSize();
					const auto cof = gSel->GetColliderOffset();

					out.colWidth = csz.x;
					out.colHeight = csz.y;
					out.colOffsetX = cof.x;
					out.colOffsetY = cof.y;

					// Tagging by special IDs (project-specific convenience)
					if (selectedObjectId == scene.GetPlayerID()) {
						out.tag = "player";
					}
					else if (selectedObjectId == scene.GetNPC1ID()) {
						out.tag = "npc1";
					}
					else if (selectedObjectId == scene.GetNPC2ID()) {
						out.tag = "npc2";
					}
					else if (selectedObjectId == scene.GetDinoID()) {
						out.tag = "dino";
					}

					// NPC velocity (store even if unused by static objects)
					const glm::vec2 v = scene.GetNPCVelocity(selectedObjectId);
					out.speedX = v.x;
					out.speedY = v.y;

					// Animation flag
					out.animated = scene.HasAnimations(selectedObjectId);

					// Layer
					out.layer = scene.GetObjectLayer(selectedObjectId);

					// Save and refresh list
					std::string savePath = prefabPath;
					if (SavePrefabToFile(savePath, out)) {
						std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", savePath.c_str());
						sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);

						// Link this instance to the prefab we just saved
						PrefabLinkByID[selectedObjectId] = NormalizePrefabPath(savePath);
					}
				}
			}
		}

		if (editor.IsPlaying()) {
			ImGui::EndDisabled();
		}

		// Instantiate from prefab (disabled if path not found)
		ImGui::BeginDisabled(!prefabExists);
		if (ImGui::Button("Instantiate from prefab")) {
			LevelObject data{};
			if (LoadPrefabFromFile(prefabPath, data)) {
				GameObject* g = nullptr;

				if (data.animated) {
					// Minimal UV set for animated sprite; your attach logic can expand this
					const std::vector<glm::vec4> uvs = { glm::vec4(0.f, 0.f, 1.f, 1.f) };
					g = scene.SpawnAnimatedSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						uvs,
						0.25f,
						true,
						data.layer
					);

					// Optional: project-specific animation attach
					if (data.texture.find("dino") != std::string::npos) {
						scene.AttachDinoAnimations(g->GetID());
						scene.SetAnimation(g->GetID(), "IDLE");
					}
				}
				else {
					g = scene.SpawnStaticSprite(
						data.texture,
						{ data.x, data.y, data.z },
						{ data.w, data.h },
						data.layer
					);
				}

				if (g != nullptr) {
					// Apply rotation (JSON stores degrees; engine expects radians)
					g->SetRotation(glm::radians(data.rotation), { 0, 0, 1 });

					// Collider
					g->SetColliderSize({ data.colWidth,  data.colHeight });
					g->SetColliderOffset({ data.colOffsetX, data.colOffsetY });

					// Store back into scene helpers for consistency
					scene.SetObjectTexturePath(g->GetID(), data.texture);
					scene.SetTransformFromLevel(
						g->GetID(),
						{ data.x, data.y, data.z },
						{ data.w, data.h, 1.0f },
						data.rotation // degrees
					);
					scene.SetNPCVelocity(g->GetID(), data.speedX, data.speedY);

					// Re-assign special IDs if the prefab carries a tag
					if (data.tag == "player") {
						scene.SetPlayerID(g->GetID());
					}
					else if (data.tag == "npc1") {
						scene.SetNPC1ID(g->GetID());
					}
					else if (data.tag == "npc2") {
						scene.SetNPC2ID(g->GetID());
					}
					else if (data.tag == "dino") {
						scene.SetDinoID(g->GetID());
					}

					// Keep within walkable area
					scene.ClampToWalkArea(g);

					// Link instance to prefab path for propagation
					PrefabLinkByID[g->GetID()] = NormalizePrefabPath(prefabPath);
				}
			}
		}

		ImGui::EndDisabled(); // !prefabExists

		// Propagate prefab changes to all instances linked to this prefab path
		if (ImGui::Button("Propagate prefab changes")) {
			if (prefabExists && selectedObjectId >= 0) {
				GameObject* src = scene.GetGameObjectByID(selectedObjectId);
				if (src) {
					const std::string normalizedPrefabPath = NormalizePrefabPath(prefabPath);

					// Build prefab based on UPDATED editor values
					LevelObject updated = BuildPrefabFromObject(scene, src);
					updated.prefabPath = normalizedPrefabPath;

					// Save updated prefab JSON
					SavePrefabToFile(prefabPath, updated);

					// Apply to all linked instances in currently open scene
					std::vector<GameObject*> objs;
					scene.CollectRenderablePointers(objs);

					int updatedCurrentScene = 0;
					for (auto* g : objs) {
						if (!g) continue;
						const int gid = g->GetID();
						auto it = PrefabLinkByID.find(gid);

						if (it != PrefabLinkByID.end() && IsSamePrefabPath(it->second, normalizedPrefabPath)) {
							ApplyPrefabToObjectKeepPosition(updated, scene, g);
							it->second = normalizedPrefabPath;
							++updatedCurrentScene;
						}
					}

					const int updatedAcrossLevels = PropagatePrefabToAllLevelFiles(normalizedPrefabPath, updated);
					std::cout << "[Prefab] Propagated '" << normalizedPrefabPath << "' to "
						<< updatedCurrentScene << " live objects and "
						<< updatedAcrossLevels << " saved level objects." << std::endl;
				}
			}
		}

		ImGui::End();
	}
#else
	// Release build: no-op implementation to avoid ImGui dependency
	void DrawPrefabsPanel(LevelEditor& /*editor*/, Scene& /*scene*/, int& /*selectedObjectId*/) {
		// Prefab editor disabled in Release builds.
	}
#endif

}
