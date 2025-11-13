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

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

using namespace LEFILEIO;

namespace LEPANELASSETS {
	// Draw the Assets docked window
	void DrawAssetsPanel(LevelEditor& editor, Scene& scene, int& selectedIndex, int selectedObjectId) {
		// Dock into the main dockspace on first use (safe no-op otherwise)
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Assets###LE_Assets")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Assets");

		ImGui::BeginChild("##AssetsBox", ImVec2(0, 0), true);

		// Helper to gather audio from both ../assets and ../assets/Audio
		auto BuildAudioList = []() {
			std::vector<std::string> all;

			{
				auto root = ListAssetsWithExt("../assets", { ".wav" });
				all.insert(all.end(), root.begin(), root.end());
			}
			{
				auto sub = ListAssetsWithExt("../assets/Audio", { ".wav" });
				all.insert(all.end(), sub.begin(), sub.end());
			}

			// Sort + dedupe for stable ordering
			std::sort(all.begin(), all.end());
			all.erase(std::unique(all.begin(), all.end()), all.end());

			return all;
			};

		// Static caches for file lists (refresh when importing or on demand)
		static std::vector<std::string> sTextures =
			ListAssetsWithExt("../assets", { ".png", ".jpg", ".jpeg" });

		static std::vector<std::string> sPrefabs =
			ListAssetsWithExt("../prefabs", { ".json" });

		// Audio (.wav) across assets + assets/Audio
		static std::vector<std::string> sAudio = BuildAudioList();

		// Cache to avoid reloading preview textures every frame
		static std::unordered_map<std::string, Texture*> sTexturePreviewCache;

		// Cache for prefab thumbnails (keyed by prefab JSON path)
		static std::unordered_map<std::string, Texture*> sPrefabPreviewCache;

		// Audio icon for audio assets
		static Texture* sAudioIcon = nullptr;

		// State for audio import error popup
		static bool sAudioErrorPending = false;
		static bool sAudioPopupOpen = true;
		static std::string sAudioErrorMessage;

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

					if (!skipAutoApply && selectedObjectId != -1) {
						GameObject* obj = scene.GetGameObjectByID(selectedObjectId);
						if (obj) {
							const int id = obj->GetID();

							scene.SetObjectTexturePath(id, projectPath);

							if (Texture* tex = LoadTextureBypassingCache(projectPath)) {
								obj->SetTexture(tex);

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

					// Invalidate prefab preview thumbnails so they reload
					sPrefabPreviewCache.clear();
				}
			}
		}

		ImGui::SameLine();

		// Import Audio (.wav only)
		if (ImGui::Button("Import Audio...")) {
			const std::string picked =
				OpenFileDialog("All files\0*.*\0");

			if (!picked.empty()) {
				// Extract extension from picked path
				std::string ext;
				const size_t dot = picked.find_last_of('.');
				if (dot != std::string::npos) {
					ext = picked.substr(dot);
				}

				std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				// Our engine/editor only supports .wav; anything else is blocked
				if (ext != ".wav") {
					sAudioErrorMessage =
						"Unsupported audio file type: \"" + ext +
						"\".\n\nOnly .wav audio files are supported by this editor.";
					sAudioErrorPending = true;
				}
				else {
					// Copy into project audio folder
					const std::string projPath =
						CopyFileIntoProjectUnique(picked, "../assets/Audio");

					if (!projPath.empty()) {
						// Refresh audio list after copy
						sAudio = BuildAudioList();
					}
				}
			}
		}

		// Audio import error popup (standalone, centered)
		if (sAudioErrorPending) {
			ImGui::OpenPopup("Audio Import Error");
			sAudioPopupOpen = true;
			sAudioErrorPending = false;
		}

		if (ImGui::BeginPopupModal(
			"Audio Import Error",
			&sAudioPopupOpen,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			// Center the popup on first appear
			const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetWindowPos(center, ImGuiCond_Appearing);

			ImGui::TextWrapped("%s", sAudioErrorMessage.c_str());
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Center the OK button
			ImGui::SetCursorPosX(
				ImGui::GetCursorPosX() +
				(ImGui::GetContentRegionAvail().x - 120.0f) * 0.5f);

			if (ImGui::Button("OK", ImVec2(120.0f, 0.0f))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
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

					if (selectedObjectId != -1) {
						GameObject* o = scene.GetGameObjectByID(selectedObjectId);
						if (o) {
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
			if (ImGui::Button("Refresh##pf")) {
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
				sPrefabPreviewCache.clear();
			}

			bool refreshPrefabs = false;

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
						// Reuse the same texture loader as the texture list
						previewTex = LoadTextureBypassingCache(data.texture);
					}

					// Cache even nullptr so we don’t keep trying failed loads
					sPrefabPreviewCache[path] = previewTex;
				}

				const float iconSize = 32.0f;

				// Draw prefab sprite thumbnail (if any), then the file name
				if (previewTex) {
					ImTextureID texID = (ImTextureID)(intptr_t)previewTex->GetID();
					ImGui::Image(
						texID,
						ImVec2(iconSize, iconSize),
						ImVec2(0, 1),
						ImVec2(1, 0)
					);
					ImGui::SameLine();
				}

				ImGui::Selectable(path.c_str(), false, 0, ImVec2(0.0f, iconSize));

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
							// Also drop the cached thumbnail for this prefab
							sPrefabPreviewCache.erase(path);
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshPrefabs) {
				sPrefabs = ListAssetsWithExt("../prefabs", { ".json" });
				sPrefabPreviewCache.clear();
			}
		}

		// Audio section
		if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Refresh##audio")) {
				sAudio = BuildAudioList();
			}

			// Lazy-load a generic icon once (placeholder)
			if (!sAudioIcon) {
				sAudioIcon = LoadTextureBypassingCache("../assets/mc_sprite_front.png");
			}

			bool refreshAudio = false;
			const float iconSize = 32.0f;

			for (const auto& path : sAudio) {
				ImGui::PushID(path.c_str());

				// Draw icon, same style as textures/prefabs
				if (sAudioIcon) {
					ImTextureID texID = (ImTextureID)(intptr_t)sAudioIcon->GetID();
					ImGui::Image(
						texID,
						ImVec2(iconSize, iconSize),
						ImVec2(0, 1),
						ImVec2(1, 0));
					ImGui::SameLine();
				}

				// Make the selectable at least as tall as the icon
				ImGui::Selectable(path.c_str(), false, 0, ImVec2(0.0f, iconSize));

				// Drag source so other panels can receive audio
				if (ImGui::BeginDragDropSource()) {
					ImGui::SetDragDropPayload("AUDIO_PATH", path.c_str(), path.size() + 1);
					ImGui::TextUnformatted("Audio");
					ImGui::TextWrapped("%s", path.c_str());
					ImGui::EndDragDropSource();
				}

				// Right-click context menu: soft delete
				if (ImGui::BeginPopupContextItem(
					(std::string("ctx_audio##") + path).c_str())) {
					if (ImGui::MenuItem("Delete")) {
						if (MoveToTrash(path)) {
							refreshAudio = true;
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			if (refreshAudio) {
				sAudio = BuildAudioList();
			}
		}

		ImGui::EndChild();
		ImGui::End();
		(void)editor; // currently unused in this panel; keep parameter for future hooks
	}
}
