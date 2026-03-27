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

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorFileIO.hpp"
#include "EngineCore/LevelEditorPanelPrefabs.hpp"
#include "EngineCore/LevelEditorPrefabLinks.hpp"
#include "EngineCore/LevelSerializer.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

#ifdef _DEBUG
#include <imgui.h>
#endif

namespace fs = std::filesystem;

using namespace LEFILEIO;
using namespace LELINKS;

namespace LEPANELPREFABS {
#ifdef _DEBUG
	/**
	 * @brief Builds a serializable prefab snapshot from a scene object.
	 * @param scene Scene containing the object.
	 * @param g Object to serialize.
	 * @return LevelObject snapshot representing the object.
	 */
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

	/**
	 * @brief Compares two floating-point values using a tolerance.
	 * @param a First value.
	 * @param b Second value.
	 * @param epsilon Allowed absolute difference.
	 * @return True when the values are within the requested tolerance.
	 */
	static bool NearlyEqual(float a, float b, float epsilon = 0.01f) {
		return std::fabs(a - b) <= epsilon;
	}

	/**
	 * @brief Returns whether two prefab snapshots are equivalent for propagation checks.
	 * @param lhs First prefab snapshot.
	 * @param rhs Second prefab snapshot.
	 * @return True when the compared prefab properties are effectively equal.
	 */
	static bool IsPrefabEquivalent(const LevelObject& lhs, const LevelObject& rhs) {
		return lhs.texture == rhs.texture &&
			lhs.tag == rhs.tag &&
			lhs.layer == rhs.layer &&
			NearlyEqual(lhs.x, rhs.x) &&
			NearlyEqual(lhs.y, rhs.y) &&
			NearlyEqual(lhs.z, rhs.z) &&
			NearlyEqual(lhs.w, rhs.w) &&
			NearlyEqual(lhs.h, rhs.h) &&
			NearlyEqual(lhs.rotation, rhs.rotation) &&
			NearlyEqual(lhs.colWidth, rhs.colWidth) &&
			NearlyEqual(lhs.colHeight, rhs.colHeight) &&
			NearlyEqual(lhs.colOffsetX, rhs.colOffsetX) &&
			NearlyEqual(lhs.colOffsetY, rhs.colOffsetY) &&
			NearlyEqual(lhs.speedX, rhs.speedX) &&
			NearlyEqual(lhs.speedY, rhs.speedY) &&
			(lhs.animated == rhs.animated);
	}

	/**
	 * @brief Ensures that a prefab path ends with a `.json` extension.
	 * @param path Path string to normalize in place.
	 */
	static inline void EnsureJsonExt(std::string& path) {
		if (fs::path(path).extension().empty()) {
			path += ".json";
		}
	}

	/**
	 * @brief Resolves user-entered prefab text into a normalized prefab path.
	 * @param inputPath User-supplied prefab path or filename.
	 * @return Resolved prefab path with a `.json` extension.
	 */
	static std::string ResolvePrefabPathFromInput(const std::string& inputPath) {
		fs::path resolved(inputPath);

		if (!resolved.has_parent_path()) {
			resolved = fs::path(FilePaths::Dirs::PREFABS_EDITOR) / resolved;
		}

		std::string out = resolved.generic_string();
		EnsureJsonExt(out);
		return out;
	}

	/**
	 * @brief Normalizes a prefab path for reliable comparisons.
	 * @param path Prefab path to normalize.
	 * @return Canonicalized or lexically normalized prefab path.
	 */
	static std::string NormalizePrefabPath(const std::string& path) {
		std::error_code ec;
		const fs::path raw(path);
		const fs::path canonical = fs::weakly_canonical(raw, ec);
		if (!ec) {
			return canonical.lexically_normal().generic_string();
		}

		return raw.lexically_normal().generic_string();
	}

	/**
	 * @brief Returns whether two prefab paths refer to the same file.
	 * @param lhs First prefab path.
	 * @param rhs Second prefab path.
	 * @return True when both paths resolve to the same prefab file.
	 */
	static bool IsSamePrefabPath(const std::string& lhs, const std::string& rhs) {
		if (lhs == rhs) {
			return true;
		}

		return NormalizePrefabPath(lhs) == NormalizePrefabPath(rhs);
	}

	/**
	 * @brief Resolves the levels directory used when propagating prefab edits to disk.
	 * @return Directory path containing level JSON files.
	 */
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

	/**
	 * @brief Propagates an updated prefab snapshot to all linked objects across level files.
	 * @param prefabPath Prefab file path being updated.
	 * @param updatedPrefab New prefab data to apply.
	 * @return Number of linked objects updated across all level files.
	 */
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

	/**
	 * @brief Draws the docked Prefabs panel for saving and instantiating prefabs.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param selectedObjectId Engine object ID for the active scene selection.
	 */
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
		auto GetPrefabDisplayName = [](const std::string& prefabPath) {
			const std::string filename = fs::path(prefabPath).filename().string();
			return filename.empty() ? prefabPath : filename;
			};

		// Cache for prefab thumbnails (keyed by prefab JSON path)
		static std::unordered_map<std::string, Texture*> sPrefabPreviewCache;

		ImGui::TextUnformatted("Prefab path");
		ImGui::SameLine();

		if (ImGui::Button("Refresh##pf")) {
			sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);
			sPrefabPreviewCache.clear();
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
					sPrefabPreviewCache.clear();

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

		std::string selectedPrefabLabel = GetPrefabDisplayName(prefabPathBuf);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##PrefabCombo", selectedPrefabLabel.c_str())) {
			for (size_t i = 0; i < sPrefabs.size(); ++i) {
				const std::string displayName = GetPrefabDisplayName(sPrefabs[i]);
				bool selected = (sPrefabs[i] == prefabPathBuf);
				const std::string comboLabel = displayName + "##combo_prefab_" + std::to_string(i);
				if (ImGui::Selectable(comboLabel.c_str(), selected)) {
					std::snprintf(prefabPathBuf, sizeof(prefabPathBuf), "%s", sPrefabs[i].c_str());
				}

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
					ImGui::SetTooltip("%s", sPrefabs[i].c_str());
				}

				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##PrefabPathEdit", prefabPathBuf, IM_ARRAYSIZE(prefabPathBuf));

		std::string prefabPath = ResolvePrefabPathFromInput(prefabPathBuf);
		const bool prefabExists = fs::exists(prefabPath);

		// Prefab list
		ImGui::Spacing();
		ImGui::SeparatorText("Prefab Library");

		if (ImGui::Button("Refresh##pf_list")) {
			sPrefabs = ListJsonFiles(FilePaths::Dirs::PREFABS_EDITOR);
			sPrefabPreviewCache.clear();
		}

		static char prefabFilterBuf[128] = {};
		static bool showOnlyMatchingPrefabs = true;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##PrefabFilter", "Filter prefabs...", prefabFilterBuf, IM_ARRAYSIZE(prefabFilterBuf));
		ImGui::Checkbox("Show only matching names", &showOnlyMatchingPrefabs);

		const std::string prefabFilter = prefabFilterBuf;
		const bool hasPrefabFilter = !prefabFilter.empty();
		auto matchesPrefabFilter = [&](const std::string& value) {
			if (!hasPrefabFilter) {
				return true;
			}

			const std::string filterTarget = GetPrefabDisplayName(value);

			auto it = std::search(
				filterTarget.begin(), filterTarget.end(),
				prefabFilter.begin(), prefabFilter.end(),
				[](char lhs, char rhs) {
					return std::tolower(static_cast<unsigned char>(lhs)) ==
						std::tolower(static_cast<unsigned char>(rhs));
				});
			return it != filterTarget.end();
			};

		// Scrollable area for prefab thumbnails + paths
		ImGui::BeginChild("##PrefabList", ImVec2(0, 200.0f), true);

		bool refreshPrefabs = false;
		const float iconSize = 32.0f;
		int visiblePrefabCount = 0;

		for (const auto& path : sPrefabs) {
			const bool matchesFilter = matchesPrefabFilter(path);
			if (showOnlyMatchingPrefabs && !matchesFilter) {
				continue;
			}

			++visiblePrefabCount;

			ImGui::PushID(path.c_str());
			if (hasPrefabFilter && !matchesFilter) {
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.45f);
			}

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
			const std::string displayName = GetPrefabDisplayName(path);
			if (ImGui::Selectable(displayName.c_str(), isSelected,
				0, ImVec2(0.0f, iconSize))) {
				// Clicking on list item updates the active prefab path
				std::snprintf(prefabPathBuf,
					sizeof(prefabPathBuf),
					"%s", path.c_str());
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("%s", path.c_str());
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

			if (hasPrefabFilter && !matchesFilter) {
				ImGui::PopStyleVar();
			}

			ImGui::PopID();
		}

		if (visiblePrefabCount == 0) {
			ImGui::TextDisabled("No prefabs match filter.");
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

		auto ShowButtonTooltip = [](const char* message) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
				ImGui::TextUnformatted(message);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
			};

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

					// Transform (position/scale) store rotation in DEGREES for JSON
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
						sPrefabPreviewCache.erase(savePath);

						// Link this instance to the prefab we just saved
						PrefabLinkByID[selectedObjectId] = NormalizePrefabPath(savePath);
					}
				}
			}
		}

		ShowButtonTooltip("Overwrite/create prefab from selected object.");

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
					if (g && data.texture.find("dino") != std::string::npos) {
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

					// Re-assign special IDs via tag rules hook
					scene.ApplyTagRules(g->GetID(), data.tag, data.speedX, data.speedY);

					// Keep within walkable area
					scene.ClampToWalkArea(g);

					// Link instance to prefab path for propagation
					PrefabLinkByID[g->GetID()] = NormalizePrefabPath(prefabPath);
				}
			}
		}

		ShowButtonTooltip("Spawn a new object from selected prefab file.");

		ImGui::EndDisabled(); // !prefabExists

		const auto propagatePrefabChanges = [&]() {
			if (!prefabExists || selectedObjectId < 0) {
				return;
			}

			GameObject* src = scene.GetGameObjectByID(selectedObjectId);
			if (!src) {
				return;
			}

			const std::string normalizedPrefabPath = NormalizePrefabPath(prefabPath);

			// Build prefab based on UPDATED editor values
			LevelObject updated = BuildPrefabFromObject(scene, src);
			updated.prefabPath = normalizedPrefabPath;

			LevelObject currentPrefab{};
			const bool hasCurrentPrefab = LoadPrefabFromFile(prefabPath, currentPrefab);

			// Save updated prefab JSON only when prefab content changed.
			if (!hasCurrentPrefab || !IsPrefabEquivalent(currentPrefab, updated)) {
				SavePrefabToFile(prefabPath, updated);
			}

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

			// Apply to all linked instances in other level files.
			PropagatePrefabToAllLevelFiles(normalizedPrefabPath, updated);
			};

		// Propagate prefab changes to all instances linked to this prefab path
		if (ImGui::Button("Propagate prefab changes")) {
			ImGui::OpenPopup("Confirm Propagate Prefab");
		}

		ShowButtonTooltip("Update all linked instances in open scene + level files.");

		if (ImGui::BeginPopupModal("Confirm Propagate Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Apply prefab changes to all linked instances?");
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
			ImGui::Text("Prefab: %s", prefabPath.c_str());
			ImGui::PopTextWrapPos();
			ImGui::Spacing();

			GameObject* src = (selectedObjectId >= 0) ? scene.GetGameObjectByID(selectedObjectId) : nullptr;
			const bool canPropagate = prefabExists && (src != nullptr);
			if (!canPropagate) {
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.25f, 1.0f),
					"Select a valid object and prefab path before propagating.");
				ImGui::Spacing();
			}

			ImGui::Separator();
			if (!canPropagate) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Propagate", ImVec2(120.0f, 0.0f))) {
				propagatePrefabChanges();
				ImGui::CloseCurrentPopup();
			}
			if (!canPropagate) {
				ImGui::EndDisabled();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}
#else
	/**
	 * @brief Draws prefabs panel.
	 */
	void DrawPrefabsPanel(LevelEditor& /*editor*/, Scene& /*scene*/, int& /*selectedObjectId*/) {
		// Prefab editor disabled in Release builds.
	}
#endif

}
