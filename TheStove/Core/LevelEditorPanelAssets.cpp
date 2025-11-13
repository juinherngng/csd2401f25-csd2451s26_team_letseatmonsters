/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAssets.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Implementation of the Level Editor Assets panel.
					- Import Texture / Import Prefab (native dialog)
					- Refreshable lists for textures and prefabs
					- Drag & drop payloads ("ASSET_PATH", "PREFAB_PATH")
					- Double-click a texture to apply to the selected object
					- Context menu: soft delete (move to "trash")

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorPanelAssets.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"

#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GraphicsEngine.hpp"

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

using namespace LEFILEIO;

namespace LEPANELASSETS {
	// Draw the Assets docked window
	void DrawAssetsPanel(LevelEditor& editor, Scene& scene, int& selectedIndex) {
		// Dock into the main dockspace on first use (safe no-op otherwise)
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Assets###LE_Assets")) {
			ImGui::End();
			return;
		}

		ImGui::Text("Assets");
		ImGui::Spacing();

		ImGui::BeginChild("##AssetsBox", ImVec2(0, 0), true);

		// Static caches for file lists (refresh when importing or on demand)
		static std::vector<std::string> sTextures =
			ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });

		static std::vector<std::string> sPrefabs =
			ListAssetsWithExt("../prefabs", { ".json" });

		// Cache to avoid reloading preview textures every frame
		static std::unordered_map<std::string, Texture*> sTexturePreviewCache;

		// Import row
		if (ImGui::Button("Import Texture...")) {
			const std::string pickedPath =
				OpenFileDialog("PNG files\0*.png\0All files\0*.*\0");

			if (!pickedPath.empty()) {
				const std::string projectPath =
					CopyFileIntoProjectUnique(pickedPath, "../assets");

				if (!projectPath.empty()) {
					// Refresh list after copy
					sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });

					// Auto-apply to currently selected object unless Ctrl is held
					ImGuiIO& io = ImGui::GetIO();
					const bool skipAutoApply = io.KeyCtrl
						|| ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
						|| ImGui::IsKeyDown(ImGuiKey_RightCtrl);

					if (!skipAutoApply) {
						std::vector<GameObject*> objectList;
						scene.CollectRenderablePointers(objectList);

						if (selectedIndex >= 0 &&
							selectedIndex < static_cast<int>(objectList.size()) &&
							objectList[selectedIndex] != nullptr) {
							GameObject* obj = objectList[selectedIndex];
							const int id = obj->GetID();

							scene.SetObjectTexturePath(id, projectPath);

							if (Texture* tex = LoadTextureBypassingCache(projectPath)) {
								obj->SetTexture(tex);

								// Optional: project-specific animation tagging
								if (projectPath.find("dino_") != std::string::npos) {
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
					}
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Prefab...")) {
			const std::string picked =
				OpenFileDialog("JSON files\0*.json\0All files\0*.*\0");

			if (!picked.empty()) {
				const std::string projPath =
					CopyFileIntoProjectUnique(picked, "../prefabs");

				if (!projPath.empty()) {
					// Refresh list after copy
					sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
				}
			}
		}

		ImGui::Separator();

		// Textures section
		if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##tex")) {
				sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
			}

			bool refreshTextures = false;

			for (const auto& path : sTextures) {
				ImGui::PushID(path.c_str());

				// Fetch or load preview texture for this path
				Texture* previewTex = nullptr;
				auto it = sTexturePreviewCache.find(path);
				if (it != sTexturePreviewCache.end()) {
					previewTex = it->second;
				}
				else {
					// Use your existing loader
					previewTex = LoadTextureBypassingCache(path);
					sTexturePreviewCache[path] = previewTex;
				}

				const float iconSize = 32.0f;

				// If we have a texture, draw its image first
				if (previewTex) {
					ImTextureID texID = (ImTextureID)(intptr_t)previewTex->GetID();

					// Draw the thumbnail (UVs flipped vertically for OpenGL)
					ImGui::Image(texID,
						ImVec2(iconSize, iconSize),
						ImVec2(0, 1),
						ImVec2(1, 0));

					ImGui::SameLine();
				}

				// Make the selectable at least as tall as the icon so they line up nicely
				ImGui::Selectable(path.c_str(), false, 0, ImVec2(0.0f, iconSize));

				// Double-click to apply to current selection
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
					ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {

					std::vector<GameObject*> objs;
					scene.CollectRenderablePointers(objs);

					if (selectedIndex >= 0 && selectedIndex < static_cast<int>(objs.size()) && objs[selectedIndex]) {
						GameObject* o = objs[selectedIndex];
						const int id2 = o->GetID();

						scene.SetObjectTexturePath(id2, path);

						if (auto* tex = LoadTextureBypassingCache(path)) {
							o->SetTexture(tex);

							if (path.find("dino_") != std::string::npos) {
								scene.AttachDinoAnimations(id2);
								scene.SetAnimation(id2, "IDLE");
								scene.MarkAnimated(id2, true);
							}
							else {
								o->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
								scene.MarkAnimated(id2, false);
							}
						}
					}
				}

				// Drag source payload for other panels/targets
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("ASSET_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Texture");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}

				// Context menu: soft delete (move to /trash)
				if (ImGui::BeginPopupContextItem((std::string("ctx_tex##") + path).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						if (MoveToTrash(path)) {
							refreshTextures = true;
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshTextures) {
				sTextures = ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });
			}
		}

		// Prefabs section
		if (ImGui::CollapsingHeader("Prefabs", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##pf"))
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });

			bool refreshPrefabs = false;

			for (const auto& path : sPrefabs) {
				ImGui::PushID(path.c_str());

				ImGui::Selectable(path.c_str(), false);

				// Drag source (instantiate/apply in Level panel targets)
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("PREFAB_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Prefab");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}

				// Context menu: soft delete
				if (ImGui::BeginPopupContextItem((std::string("ctx_prefab##") + path).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						if (MoveToTrash(path)) {
							refreshPrefabs = true;
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshPrefabs) {
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
			}
		}

		ImGui::EndChild();
		ImGui::End();
		(void)editor; // currently unused in this panel; keep parameter for future hooks
	}
}
