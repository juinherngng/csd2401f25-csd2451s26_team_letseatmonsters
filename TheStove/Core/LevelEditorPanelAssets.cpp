/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAssets.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:       Implementation of the Level Editor Assets panel.
					- Import Texture / Import Prefab / Import Audio (native dialog)
					- Refreshable lists for textures, prefabs, and audio
					- Drag & drop payloads ("ASSET_PATH", "PREFAB_PATH", "AUDIO_PATH")
					- Double-click a texture to apply to the selected object
					- Context menu: soft delete (move to "trash")
					- Audio catalog management with inline editing

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "LevelEditorPanelAssets.hpp"

#include "LevelEditor.hpp"
#include "LevelEditorFileIO.hpp"
#include "AudioLoading.hpp"

#include "../Graphics/GameObject.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

using namespace LEFILEIO;

namespace {
	// Helper to normalize paths for audio loading (convert to forward slashes)
	std::string NormalizeAudioPath(const std::string& path) {
		std::string normalized = path;
		
		// Convert backslashes to forward slashes
		std::replace(normalized.begin(), normalized.end(), '\\', '/');
		
		// Try to make it absolute if it's relative and doesn't exist at the current location
		try {
			fs::path inputPath(normalized);
			
			// If it's a relative path, try to resolve it
			if (inputPath.is_relative()) {
				fs::path absPath = fs::absolute(inputPath);
				
				// If the absolute path doesn't exist, try to find it relative to the executable
				if (!fs::exists(absPath)) {
					// Try to construct path relative to the build directory
					fs::path buildPath = fs::current_path() / "build" / normalized;
					if (fs::exists(buildPath)) {
						absPath = buildPath;
					}
					// Try without "build" folder
					else {
						fs::path reposPath = fs::current_path();
						// Navigate up to find the repos folder
						while (!reposPath.empty() && reposPath.filename() != "repos") {
							reposPath = reposPath.parent_path();
						}
						if (!reposPath.empty()) {
							fs::path testPath = reposPath / "csd2401f25-csd2451s26_team_letseatmonsters" / "build" / normalized;
							if (fs::exists(testPath)) {
								absPath = testPath;
							}
						}
					}
				}
				
				normalized = absPath.string();
				// Convert to forward slashes again
				std::replace(normalized.begin(), normalized.end(), '\\', '/');
			}
		} catch (...) {
			// If any error occurs, keep the original normalized path
		}
		
		return normalized;
	}
}

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
				auto root = ListAssetsWithExt("../assets", { ".wav", ".mp3" });
				all.insert(all.end(), root.begin(), root.end());
			}
			{
				auto sub = ListAssetsWithExt("../assets/Audio", { ".wav", ".mp3" });
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

		// Audio (.wav, .mp3) across assets + assets/Audio
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

		// Import Audio (.wav and .mp3 supported)
		if (ImGui::Button("Import Audio...")) {
			const std::string picked =
				OpenFileDialog("Audio Files\0*.wav;*.mp3\0WAV Files\0*.wav\0MP3 Files\0*.mp3\0All Files\0*.*\0");

			if (!picked.empty()) {
				std::cout << "[Assets Panel] Selected file: " << picked << std::endl;
				
				// Extract extension from picked path
				std::string ext;
				const size_t dot = picked.find_last_of('.');
				if (dot != std::string::npos) {
				ext = picked.substr(dot);
				}

				std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				// Support both .wav and .mp3 formats
				if (ext != ".wav" && ext != ".mp3") {
					sAudioErrorMessage =
						"Unsupported audio file type: \"" + ext +
						"\".\n\nOnly .wav and .mp3 audio files are supported by this editor.";
					sAudioErrorPending = true;
				}
				else {
					std::cout << "[Assets Panel] Copying to: ../assets/Audio" << std::endl;
					
					// Copy into project audio folder
					const std::string projPath =
						CopyFileIntoProjectUnique(picked, "../assets/Audio");

					if (projPath.empty()) {
						std::cerr << "[Assets Panel] ERROR: Failed to copy file!" << std::endl;
					}
					else {
						std::cout << "[Assets Panel] File copied to: " << projPath << std::endl;
						
						// Normalize the path for FMOD (convert backslashes to forward slashes)
						const std::string normalizedPath = NormalizeAudioPath(projPath);
						std::cout << "[Assets Panel] Normalized path: " << normalizedPath << std::endl;

						// Refresh audio list after copy
						sAudio = BuildAudioList();

						// Auto-add to catalog if not already present
						if (Audio::AudioCatalog::IsValidAudioFile(normalizedPath)) {
							// Create a new audio asset
							Audio::AudioAsset newAsset;
							newAsset.filepath = normalizedPath;

							// Extract name from filepath (without extension)
							size_t lastSlash = normalizedPath.find_last_of("/\\");
							size_t lastDot = normalizedPath.find_last_of('.');

							if (lastSlash != std::string::npos && lastDot != std::string::npos) {
								newAsset.name = normalizedPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
							}
							else {
								const auto& assets = Audio::AudioCatalog::GetAllAssets();
								newAsset.name = "audio_" + std::to_string(assets.size());
							}

							std::cout << "[Assets Panel] Adding to catalog as: " << newAsset.name << std::endl;

							// Default properties
							newAsset.loop = false;
							newAsset.stream = false;
							newAsset.category = "sfx";
							newAsset.volume = 1.0f;

							// Add to catalog
							if (Audio::AudioCatalog::AddAudioAsset(newAsset)) {
								std::cout << "[Assets Panel] Successfully added to catalog" << std::endl;
								
								// Load the audio
								ResourceManager::Instance().LoadAudio(
									newAsset.name,
									newAsset.filepath,
									newAsset.loop,
									newAsset.stream
								);
								
								// Auto-save catalog after successful import
								if (Audio::AudioCatalog::SaveCatalogToFile("../assets/Audio/AudioCatalog.json")) {
									std::cout << "[Assets Panel] Catalog auto-saved successfully" << std::endl;
								}
								else {
									std::cerr << "[Assets Panel] WARNING: Failed to auto-save catalog!" << std::endl;
								}
							}
							else {
								std::cerr << "[Assets Panel] ERROR: Failed to add to catalog!" << std::endl;
							}
						}
						else {
							std::cerr << "[Assets Panel] ERROR: Invalid audio file format!" << std::endl;
						}
					}
				}
			}
			else {
				std::cout << "[Assets Panel] File selection cancelled" << std::endl;
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

					// Cache even nullptr so we don't keep trying failed loads
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

		// Audio section with catalog management
		if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Catalog management buttons
			ImGui::BeginGroup();
			if (ImGui::Button("Refresh##audio")) {
				sAudio = BuildAudioList();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save Catalog")) {
				if (Audio::AudioCatalog::SaveCatalogToFile("../assets/Audio/AudioCatalog.json")) {
					ImGui::OpenPopup("Catalog Saved");
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Reload Catalog")) {
				Audio::AudioCatalog::UnloadAllAudio();
				if (Audio::AudioCatalog::LoadCatalogFromFile("../assets/Audio/AudioCatalog.json")) {
					Audio::AudioCatalog::LoadAllAudio();
					ImGui::OpenPopup("Catalog Reloaded");
				}
			}

			// Success popups
			if (ImGui::BeginPopupModal("Catalog Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				ImGui::Text("Audio catalog saved successfully!");
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal("Catalog Reloaded", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				ImGui::Text("Audio catalog reloaded successfully!");
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::EndGroup();

			ImGui::Separator();

			// Audio Preview Drop Zone
			ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Audio Preview:");
			ImGui::TextDisabled("Drop audio files here to test playback");

			ImGui::BeginChild("##AudioPreviewDropZone", ImVec2(-FLT_MIN, 80.0f), true);
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Drop .wav or .mp3 file here to preview...");

			// Accept dropped audio for preview
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AUDIO_PATH")) {
					std::string droppedPath(static_cast<const char*>(payload->Data));

					// Find in catalog and play
					const auto& previewCatalog = Audio::AudioCatalog::GetAllAssets();
					bool foundInCatalog = false;

					for (const auto& asset : previewCatalog) {
						if (asset.filepath == droppedPath) {
							// Stop any currently playing preview
							ResourceManager::Instance().LoadAudio(
								"__preview__",
								asset.filepath,
								false,  // Don't loop preview
								false   // Load into memory for quick playback
							);

							// Use the AudioManager to play
							// Note: You'll need to get AudioManager from CoreEngine
							// For now, just indicate success
							ImGui::OpenPopup("Preview Playing");
							foundInCatalog = true;
							break;
						}
					}

					if (!foundInCatalog) {
						// Not in catalog, try to play directly
						ResourceManager::Instance().LoadAudio(
							"__preview__",
							droppedPath,
							false,
							false
						);
						ImGui::OpenPopup("Preview Playing");
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::EndChild();

			// Preview feedback popup
			if (ImGui::BeginPopupModal("Preview Playing", nullptr, 
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				ImGui::Text("Playing audio preview...");
				ImGui::TextDisabled("(Feature requires AudioManager integration)");
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// Get catalog assets
			const auto& catalogAssets = Audio::AudioCatalog::GetAllAssets();

			// Display catalog entries with inline editing
			if (!catalogAssets.empty()) {
				ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Catalog Entries (%zu):", catalogAssets.size());

				// State for inline editing
				static std::string editingName = "";
				static bool editMode = false;
				static Audio::AudioAsset editBuffer;

				for (const auto& asset : catalogAssets) {
					ImGui::PushID(asset.name.c_str());

					bool isEditing = (editMode && editingName == asset.name);

					// Collapsing header for each audio asset
					if (ImGui::TreeNode(asset.name.c_str())) {
						if (isEditing) {
							// Edit mode
							char nameBuf[128];
							strncpy_s(nameBuf, editBuffer.name.c_str(), sizeof(nameBuf) - 1);
							ImGui::SetNextItemWidth(200.0f);
							if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
								editBuffer.name = nameBuf;
							}

							// Category combo
							const char* categories[] = { "ui", "sfx", "bgm", "ambient", "other" };
							int selectedCategory = 0;
							for (int i = 0; i < 5; ++i) {
								if (editBuffer.category == categories[i]) {
									selectedCategory = i;
									break;
								}
							}
							ImGui::SetNextItemWidth(150.0f);
							if (ImGui::Combo("Category", &selectedCategory, categories, 5)) {
								editBuffer.category = categories[selectedCategory];
							}

							ImGui::Checkbox("Loop", &editBuffer.loop);
							ImGui::SameLine();
							ImGui::Checkbox("Stream", &editBuffer.stream);

							ImGui::SetNextItemWidth(200.0f);
							ImGui::SliderFloat("Volume", &editBuffer.volume, 0.0f, 1.0f, "%.2f");

							ImGui::TextWrapped("File: %s", editBuffer.filepath.c_str());

							if (ImGui::Button("Save##Edit")) {
								// Apply changes
								Audio::AudioCatalog::RemoveAudioAsset(editingName);
								Audio::AudioCatalog::AddAudioAsset(editBuffer);

								// Reload the audio
								ResourceManager::Instance().UnloadAudio(editingName);
								ResourceManager::Instance().LoadAudio(
									editBuffer.name,
									editBuffer.filepath,
									editBuffer.loop,
									editBuffer.stream
								);

								editMode = false;
								editingName = "";
							}
							ImGui::SameLine();
							if (ImGui::Button("Cancel##Edit")) {
								editMode = false;
								editingName = "";
							}
						}
						else {
							// Display mode
							ImGui::Text("Category: %s", asset.category.c_str());
							ImGui::Text("Loop: %s", asset.loop ? "Yes" : "No");
							ImGui::Text("Stream: %s", asset.stream ? "Yes" : "No");
							ImGui::Text("Volume: %.2f", asset.volume);
							ImGui::TextWrapped("File: %s", asset.filepath.c_str());

							if (ImGui::Button("Edit")) {
								editMode = true;
								editingName = asset.name;
								editBuffer = asset;
							}
							ImGui::SameLine();
							if (ImGui::Button("Remove")) {
								ImGui::OpenPopup("Confirm Remove Audio");
							}

							if (ImGui::BeginPopupModal("Confirm Remove Audio", nullptr,
								ImGuiWindowFlags_AlwaysAutoResize)) {
								ImGui::Text("Remove '%s' from catalog?", asset.name.c_str());
								ImGui::Separator();

								if (ImGui::Button("Yes", ImVec2(120, 0))) {
									Audio::AudioCatalog::RemoveAudioAsset(asset.name);
									ImGui::CloseCurrentPopup();
								}
								ImGui::SameLine();
								if (ImGui::Button("No", ImVec2(120, 0))) {
									ImGui::CloseCurrentPopup();
								}
								ImGui::EndPopup();
							}
						}

						ImGui::TreePop();
					}

					ImGui::PopID();
				}

				ImGui::Separator();
			}

			// Lazy-load a generic icon once (placeholder)
			if (!sAudioIcon) {
				sAudioIcon = LoadTextureBypassingCache("../assets/mc_sprite_front.png");
			}

			bool refreshAudio = false;
			const float iconSize = 32.0f;

			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Audio Files (%zu):", sAudio.size());

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

				// Double-click to add to catalog if not already there
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
					ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {

					// Check if already in catalog
					bool inCatalog = false;
					for (const auto& asset : catalogAssets) {
						if (asset.filepath == path) {
							inCatalog = true;
							break;
						}
					}

					if (!inCatalog) {
						// Normalize the path for FMOD
						const std::string normalizedPath = NormalizeAudioPath(path);

						// Auto-add to catalog
						Audio::AudioAsset newAsset;
						newAsset.filepath = normalizedPath;

						// Extract name from filepath
						size_t lastSlash = normalizedPath.find_last_of("/\\");
						size_t lastDot = normalizedPath.find_last_of('.');

						if (lastSlash != std::string::npos && lastDot != std::string::npos) {
							newAsset.name = normalizedPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
						}
						else {
							newAsset.name = "audio_" + std::to_string(catalogAssets.size());
						}

						newAsset.loop = false;
						newAsset.stream = false;
						newAsset.category = "sfx";
						newAsset.volume = 1.0f;

						if (Audio::AudioCatalog::AddAudioAsset(newAsset)) {
							ResourceManager::Instance().LoadAudio(
								newAsset.name,
								newAsset.filepath,
								newAsset.loop,
								newAsset.stream
							);
							
							// Auto-save catalog after successful addition
							if (Audio::AudioCatalog::SaveCatalogToFile("../assets/Audio/AudioCatalog.json")) {
								std::cout << "[Assets Panel] Catalog auto-saved after double-click add" << std::endl;
							}
						}
					}
				}

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
					if (ImGui::MenuItem("Delete File")) {
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
