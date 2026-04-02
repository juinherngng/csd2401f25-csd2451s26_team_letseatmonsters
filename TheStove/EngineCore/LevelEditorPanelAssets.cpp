/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAssets.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu		(50%)
 CO-AUTHORS:        Ng Juin Herng, juinherng.ng@digipen.edu (50%)

 DESCRIPTION:       Implementation of the Level Editor Assets panel.
					- Import Texture / Import Prefab / Import Audio (native dialog)
					- Refreshable lists for textures, prefabs, and audio
					- Drag & drop payloads ("ASSET_PATH", "PREFAB_PATH", "AUDIO_PATH")
					- Double-click a texture to apply to the selected object
					- Context menu: soft delete (move to "trash")
					- Audio catalog management with inline editing

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <cctype>
#include <chrono>
#include <future>
#include <map>
#include <unordered_set>

#include "EngineCore/ApplicationState.hpp"
#include "EngineCore/AudioLoading.hpp"
#include "EngineCore/Core.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorFileIO.hpp"
#include "EngineCore/LevelEditorPanelAssets.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/Message.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

#ifdef _DEBUG
#include <imgui.h>
#endif

namespace fs = std::filesystem;

using namespace LEFILEIO;

namespace {

	struct TextureFolderNode {
		std::map<std::string, TextureFolderNode> children;
		std::vector<std::string> assets;
	};

	/**
	 * @brief Normalizes an audio asset path to use forward slashes.
	 * @param path Input path to normalize.
	 * @return Normalized path string.
	 */
	std::string NormalizeAudioPath(const std::string& path) {
		std::string normalized = path;

		// Convert backslashes to forward slashes for FMOD-friendly relative paths.
		std::replace(normalized.begin(), normalized.end(), '\\', '/');

		// Keep relative paths as-is (don't try to make them absolute)
		// FMOD can handle relative paths just fine

		return normalized;
	}

	/**
	 * @brief Infers an audio category from a filename prefix.
	 * @param name Asset name to inspect.
	 * @return Detected category label, or `"other"` when no known prefix matches.
	 */
	std::string DetectCategoryFromName(const std::string& name) {
		// Normalize to lowercase so prefix checks are case-insensitive.
		std::string lowerName = name;
		std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		// Check for common prefixes
		if (lowerName.find("ui_") == 0) return "ui";
		if (lowerName.find("sfx_") == 0) return "sfx";
		if (lowerName.find("bgm_") == 0) return "bgm";
		if (lowerName.find("ambient_") == 0) return "ambient";

		// Default to "other" if no recognized prefix
		return "other";
	}

	/**
	 * @brief Splits an asset path into folder parts relative to the assets root.
	 * @param assetPath Relative asset path returned by the editor file helpers.
	 * @return Folder components from the assets root down to the file's parent folder.
	 */
	std::vector<std::string> GetAssetFolderParts(const std::string& assetPath) {
		std::string generic = fs::path(assetPath).parent_path().generic_string();
		const std::string editorAssetsPrefix = fs::path(FilePaths::Dirs::ASSETS_EDITOR).generic_string();
		const std::string runtimeAssetsPrefix = fs::path(FilePaths::Dirs::ASSETS).generic_string();

		auto trimPrefix = [&](const std::string& prefix) {
			if (generic.rfind(prefix, 0) == 0) {
				generic.erase(0, prefix.size());
			}
			};

		trimPrefix(editorAssetsPrefix);
		trimPrefix(runtimeAssetsPrefix);

		while (!generic.empty() && (generic.front() == '/' || generic.front() == '.')) {
			generic.erase(generic.begin());
		}

		std::vector<std::string> parts;
		if (generic.empty()) {
			return parts;
		}

		for (const auto& part : fs::path(generic)) {
			const std::string name = part.generic_string();
			if (!name.empty() && name != ".") {
				parts.push_back(name);
			}
		}

		return parts;
	}

	/**
	 * @brief Inserts a texture path into the nested folder tree used by the assets panel.
	 * @param root Root tree node for all texture folders.
	 * @param assetPath Texture asset path to insert.
	 */
	void InsertTextureIntoFolderTree(TextureFolderNode& root, const std::string& assetPath) {
		TextureFolderNode* node = &root;
		for (const std::string& part : GetAssetFolderParts(assetPath)) {
			node = &node->children[part];
		}

		node->assets.push_back(assetPath);
	}

	/**
	 * @brief Counts all textures contained in a folder tree node and its descendants.
	 * @param node Folder node to count.
	 * @return Number of texture entries stored below the node.
	 */
	int CountTextureFolderAssets(const TextureFolderNode& node) {
		int count = static_cast<int>(node.assets.size());
		for (const auto& [_, child] : node.children) {
			count += CountTextureFolderAssets(child);
		}

		return count;
	}
}

namespace LEPANELASSETS {
#ifdef _DEBUG
	/**
	 * @brief Draws the docked Assets panel for importing, browsing, and applying assets.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 * @param selectedIndex Hierarchy selection index tracked across editor panels.
	 * @param selectedObjectId Engine object ID for the active scene selection.
	 */
	void DrawAssetsPanel(LevelEditor& editor, Scene& scene, int& selectedIndex, int selectedObjectId) {
		// Dock into the main dockspace on first use (safe no-op otherwise)
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Assets###LE_Assets")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Assets");

		ImGui::BeginChild("##AssetsBox", ImVec2(0, 0), true);

		// Gather audio recursively from the assets source directory.
		auto BuildAudioList = []() {
			return ListAssetsWithExt(FilePaths::Dirs::ASSETS_EDITOR, { ".wav", ".mp3" }, true);
			};

		// Static caches for file lists (refresh when importing or on demand)
		static std::vector<std::string> sTextures =
			ListAssetsWithExt(FilePaths::Dirs::ASSETS_EDITOR, { ".png", ".jpg", ".jpeg" }, true);

		// Audio (.wav, .mp3) across assets + assets/Audio
		static std::vector<std::string> sAudio = BuildAudioList();
		static std::future<std::vector<std::string>> sTextureRefreshFuture;
		static std::future<std::vector<std::string>> sAudioRefreshFuture;
		static bool sTextureRefreshInProgress = false;
		static bool sAudioRefreshInProgress = false;

		auto TryConsumeRefreshJobs = [&]() {
			if (sTextureRefreshInProgress &&
				sTextureRefreshFuture.valid() &&
				sTextureRefreshFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
				sTextures = sTextureRefreshFuture.get();
				sTextureRefreshInProgress = false;
			}

			if (sAudioRefreshInProgress &&
				sAudioRefreshFuture.valid() &&
				sAudioRefreshFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
				sAudio = sAudioRefreshFuture.get();
				sAudioRefreshInProgress = false;
			}
			};

		auto QueueTextureRefresh = [&]() {
			if (sTextureRefreshInProgress) {
				return;
			}

			sTextureRefreshFuture = std::async(std::launch::async, []() {
				return ListAssetsWithExt(FilePaths::Dirs::ASSETS_EDITOR, { ".png", ".jpg", ".jpeg" }, true);
				});
			sTextureRefreshInProgress = true;
			};

		auto QueueAudioRefresh = [&]() {
			if (sAudioRefreshInProgress) {
				return;
			}

			sAudioRefreshFuture = std::async(std::launch::async, BuildAudioList);
			sAudioRefreshInProgress = true;
			};

		TryConsumeRefreshJobs();

		// Stable preview cache key prefix inside ResourceManager.
		// We intentionally avoid storing raw Texture* pointers in this panel,
		// because ResourceManager::Clear() can invalidate them between frames.
		static constexpr const char* kPreviewTextureKeyPrefix = "le_preview_";

		// Audio icon for audio assets
		static Texture* sAudioIcon = nullptr;

		// State for audio import error popup
		static bool sAudioErrorPending = false;
		static bool sAudioPopupOpen = true;
		static std::string sAudioErrorMessage;
		static bool sTextureCompactDensity = false;
		static bool sAudioCompactDensity = false;

		auto ShowTooltip = [](const char* text) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("%s", text);
			}
			};

		auto IconButton = [&](const char* id, const char* label, const char* tooltip) {
			const std::string buttonLabel = std::string(label) + "##" + id;
			const bool pressed = ImGui::Button(buttonLabel.c_str(), ImVec2(26.0f, 0.0f));
			ShowTooltip(tooltip);
			return pressed;
			};

		// Import row (auto-fit buttons to panel width)
		const float importSpacing = ImGui::GetStyle().ItemSpacing.x;
		const float importAvailWidth = ImGui::GetContentRegionAvail().x;
		const float importButtonWidth = std::max(110.0f, (importAvailWidth - importSpacing) * 0.5f);

		if (ImGui::Button("Import Texture", ImVec2(importButtonWidth, 0.0f))) {
			const std::string pickedPath =
				OpenFileDialog("PNG files\0*.png\0All files\0*.*\0");

			if (!pickedPath.empty()) {
				// Import to SOURCE directory (../../assets from build/Release)
				const std::string projectPath =
					CopyFileIntoProjectUnique(pickedPath, FilePaths::Dirs::ASSETS_EDITOR);

				if (!projectPath.empty()) {
					// Refresh list after copy
					QueueTextureRefresh();

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

		// Import Audio (.wav and .mp3 supported)
		if (ImGui::Button("Import Audio", ImVec2(importButtonWidth, 0.0f))) {
			const std::string picked =
				OpenFileDialog("Audio Files\0*.wav;*.mp3\0WAV Files\0*.wav\0MP3 Files\0*.mp3\0All Files\0*.*\0");

			if (!picked.empty()) {
				TS_LOG_INFO("[Assets Panel] Selected file: " << picked);

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
					// Use ../../assets/Audio to go from build/Release up to project root, then into source assets
					const std::string targetDir = FilePaths::Dirs::AUDIO_EDITOR;
					TS_LOG_INFO("[Assets Panel] Copying to: " << targetDir);

					// Copy into project audio folder (SOURCE directory, not build)
					const std::string projPath =
						CopyFileIntoProjectUnique(picked, targetDir);

					if (projPath.empty()) {
						TS_LOG_ERROR("[Assets Panel] Failed to copy file!");
					}
					else {
						TS_LOG_INFO("[Assets Panel] File copied to: " << projPath);

						// Normalize the path for FMOD (convert backslashes to forward slashes)
						const std::string normalizedPath = NormalizeAudioPath(projPath);
						TS_LOG_DEBUG("[Assets Panel] Normalized path: " << normalizedPath);

						// Refresh audio list after copy
						QueueAudioRefresh();

						// Auto-add to catalog if not already present
						if (Audio::AudioCatalog::IsValidAudioFile(normalizedPath)) {
							// Create a new audio asset
							Audio::AudioAsset newAsset;
							newAsset.filepath = normalizedPath;

							// Extract name from filepath (without extension)
							size_t lastSlash = normalizedPath.find_last_of("/\\");;
							size_t lastDot = normalizedPath.find_last_of('.');

							if (lastSlash != std::string::npos && lastDot != std::string::npos) {
								newAsset.name = normalizedPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
							}
							else {
								const auto& assets = Audio::AudioCatalog::GetAllAssets();
								newAsset.name = "audio_" + std::to_string(assets.size());
							}

							TS_LOG_INFO("[Assets Panel] Adding to catalog as: " << newAsset.name);

							// Default properties
							newAsset.loop = false;
							newAsset.stream = false;
							newAsset.category = DetectCategoryFromName(newAsset.name); // Auto-detect from name
							newAsset.volume = 1.0f;

							// Add to catalog
							if (Audio::AudioCatalog::AddAudioAsset(newAsset)) {
								TS_LOG_INFO("[Assets Panel] Successfully added to catalog");

								// Load the audio
								ResourceManager::Instance().LoadAudio(
									newAsset.name,
									newAsset.filepath,
									newAsset.loop,
									newAsset.stream
								);

								// Save catalog to SOURCE directory (../../assets from build/Release)
								const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
								if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
									TS_LOG_INFO("[Assets Panel] Catalog auto-saved to: " << catalogPath);
								}
								else {
									TS_LOG_WARN("[Assets Panel] Failed to auto-save catalog!");
								}
							}
							else {
								TS_LOG_ERROR("[Assets Panel] Failed to add to catalog!");
							}
						}
						else {
							TS_LOG_ERROR("[Assets Panel] Invalid audio file format!");
						}
					}
				}
			}
			else {
				TS_LOG_INFO("[Assets Panel] File selection cancelled");
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
			static char sTextureFilter[128] = "";
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputTextWithHint("##TextureFilter", "Search textures...", sTextureFilter, IM_ARRAYSIZE(sTextureFilter));

			if (ImGui::Button("Refresh##tex")) {
				QueueTextureRefresh();
			}

			if (sTextureRefreshInProgress) {
				ImGui::SameLine();
				ImGui::TextDisabled("Indexing textures...");
			}

			bool refreshTextures = false;
			std::string filterLower = sTextureFilter;
			std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			TextureFolderNode textureTree;
			for (const auto& path : sTextures) {
				const std::string displayName = fs::path(path).filename().string();
				const std::vector<std::string> folderParts = GetAssetFolderParts(path);
				std::string searchableText;
				for (const std::string& part : folderParts) {
					if (!searchableText.empty()) {
						searchableText += "/";
					}
					searchableText += part;
				}
				if (!searchableText.empty()) {
					searchableText += "/";
				}
				searchableText += displayName;

				if (!filterLower.empty()) {
					std::string searchableLower = searchableText;
					std::transform(searchableLower.begin(), searchableLower.end(), searchableLower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (searchableLower.find(filterLower) == std::string::npos) {
						continue;
					}
				}

				InsertTextureIntoFolderTree(textureTree, path);
			}

			const float iconBoxSize = 32.0f;
			const float textureRowHeight = iconBoxSize + ImGui::GetStyle().ItemSpacing.y;
			const float textureBrowserHeight = 320.0f;

			auto DrawTextureEntry = [&](const std::string& path) {
				const std::string displayName = fs::path(path).filename().string();
				ImGui::PushID(path.c_str());

				// Fetch or load preview texture by a deterministic key.
				// This keeps ownership in ResourceManager and avoids stale pointers
				// when resources are cleared/reloaded.
				const std::string previewKey = std::string(kPreviewTextureKeyPrefix) + path;
				Texture* previewTex = ResourceManager::Instance().LoadTexture(previewKey, path);

				if (previewTex && previewTex->GetID() != 0) {
					ImTextureID texID = (ImTextureID)(intptr_t)previewTex->GetID();
					float previewWidth = iconBoxSize;
					float previewHeight = iconBoxSize;
					const int textureWidth = previewTex->GetWidth();
					const int textureHeight = previewTex->GetHeight();

					// Fit the preview into a square box while preserving the source aspect ratio.
					if (textureWidth > 0 && textureHeight > 0) {
						const float widthScale = iconBoxSize / static_cast<float>(textureWidth);
						const float heightScale = iconBoxSize / static_cast<float>(textureHeight);
						const float scale = std::min(widthScale, heightScale);
						previewWidth = std::max(1.0f, static_cast<float>(textureWidth) * scale);
						previewHeight = std::max(1.0f, static_cast<float>(textureHeight) * scale);
					}

					// Reserve a fixed-size icon slot, then center the preview inside it.
					const ImVec2 slotMin = ImGui::GetCursorScreenPos();
					ImGui::Dummy(ImVec2(iconBoxSize, iconBoxSize));
					const float offsetX = (iconBoxSize - previewWidth) * 0.5f;
					const float offsetY = (iconBoxSize - previewHeight) * 0.5f;
					ImGui::GetWindowDrawList()->AddImage(
						texID,
						ImVec2(slotMin.x + offsetX, slotMin.y + offsetY),
						ImVec2(slotMin.x + offsetX + previewWidth, slotMin.y + offsetY + previewHeight),
						ImVec2(0, 1),
						ImVec2(1, 0));

					ImGui::SameLine();
				}
				else {
					// Keep text rows aligned even when a preview texture fails to load.
					ImGui::Dummy(ImVec2(iconBoxSize, iconBoxSize));
					ImGui::SameLine();
				}

				// Make the selectable at least as tall as the icon so they line up nicely
				ImGui::Selectable(displayName.c_str(), false, 0, ImVec2(0.0f, iconBoxSize));
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", path.c_str());
				}

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
				};

			auto DrawTextureFolderTree = [&](auto&& self, const TextureFolderNode& node, const std::string& folderName, const std::string& nodePath) -> void {
				const int assetCount = CountTextureFolderAssets(node);
				std::string header = folderName + " (" + std::to_string(assetCount) + ")";
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				if (!filterLower.empty()) {
					flags |= ImGuiTreeNodeFlags_DefaultOpen;
				}

				if (ImGui::TreeNodeEx(nodePath.c_str(), flags, "%s", header.c_str())) {
					ImGuiListClipper clipper;
					clipper.Begin(static_cast<int>(node.assets.size()), textureRowHeight);
					while (clipper.Step()) {
						for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index) {
							DrawTextureEntry(node.assets[static_cast<size_t>(index)]);
						}
					}

					for (const auto& [childName, childNode] : node.children) {
						const std::string childPath = nodePath + "/" + childName;
						self(self, childNode, childName, childPath);
					}

					ImGui::TreePop();
				}
				};

			// Keep the texture browser in its own scrolling region so large folders
			// do not spill over later sections like Audio.
			if (ImGui::BeginChild("##TextureBrowserTree", ImVec2(0.0f, textureBrowserHeight), true,
				ImGuiWindowFlags_HorizontalScrollbar)) {
				for (const auto& path : textureTree.assets) {
					DrawTextureEntry(path);
				}

				for (const auto& [folderName, folderNode] : textureTree.children) {
					DrawTextureFolderTree(DrawTextureFolderTree, folderNode, folderName, folderName);
				}
			}

			ImGui::EndChild();

			if (refreshTextures) {
				QueueTextureRefresh();
			}
		}

		// Audio section with catalog management
		if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Catalog management buttons
			ImGui::BeginGroup();
			if (ImGui::Button("Refresh##audio")) {
				QueueAudioRefresh();
			}

			if (sAudioRefreshInProgress) {
				ImGui::SameLine();
				ImGui::TextDisabled("Indexing audio...");
			}

			ImGui::SameLine();

			if (ImGui::Button("Save Catalog")) {
				// Save to SOURCE directory, not build directory
				const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
				if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
					ImGui::OpenPopup("Catalog Saved");
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Reload Catalog")) {
				Audio::AudioCatalog::UnloadAllAudio();
				// Load from SOURCE directory
				const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
				if (Audio::AudioCatalog::LoadCatalogFromFile(catalogPath)) {
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
					const std::string droppedPath = NormalizeAudioPath(static_cast<const char*>(payload->Data));
					static std::string sPreviewAudioName;

					// Find in catalog and play
					const auto& previewCatalog = Audio::AudioCatalog::GetAllAssets();
					bool foundInCatalog = false;

					for (const auto& asset : previewCatalog) {
						if (NormalizeAudioPath(asset.filepath) == droppedPath) {
							if (g_AppState && g_AppState->coreEngine) {
								if (!sPreviewAudioName.empty()) {
									g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(sPreviewAudioName);
								}

								g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>(
									asset.name,
									asset.volume,
									false
								);
								sPreviewAudioName = asset.name;
								ImGui::OpenPopup("Preview Playing");
							}
							foundInCatalog = true;
							break;
						}
					}

					if (!foundInCatalog && g_AppState && g_AppState->coreEngine) {
						if (!sPreviewAudioName.empty()) {
							g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(sPreviewAudioName);
						}

						ResourceManager::Instance().UnloadAudio("__preview__");
						if (ResourceManager::Instance().LoadAudio("__preview__", droppedPath, false, false)) {
							g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("__preview__", 1.0f, false);
							sPreviewAudioName = "__preview__";
							ImGui::OpenPopup("Preview Playing");
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::EndChild();

			// Preview feedback popup
			if (ImGui::BeginPopupModal("Preview Playing", nullptr,
				ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
				ImGui::Text("Playing audio preview...");
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// Get catalog assets
			const auto& catalogAssets = Audio::AudioCatalog::GetAllAssets();

			// Display catalog entries as a compact library browser instead of a long inline form list.
			if (!catalogAssets.empty()) {
				ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Catalog Entries (%zu):", catalogAssets.size());
				static char sAudioCatalogFilter[128] = "";
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputTextWithHint("##AudioCatalogFilter", "Search audio assets...", sAudioCatalogFilter, IM_ARRAYSIZE(sAudioCatalogFilter));
				std::string filterLower = sAudioCatalogFilter;
				std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				static Audio::AudioAsset editBuffer;
				static bool applyRemove = false;
				static std::string pendingRemoveName = "";
				static bool applyEdit = false;
				static std::string originalEditName = "";
				static Audio::AudioAsset pendingEditBuffer;
				static std::string currentlyPlaying = "";
				static bool requestOpenEditPopup = false;
				static bool requestOpenRemovePopup = false;

				std::unordered_map<std::string, std::vector<const Audio::AudioAsset*>> audioByCategory;
				for (const auto& asset : catalogAssets) {
					audioByCategory[asset.category].push_back(&asset);
				}

				for (auto& [_, assets] : audioByCategory) {
					std::sort(assets.begin(), assets.end(),
						[](const Audio::AudioAsset* a, const Audio::AudioAsset* b) {
							return a->name < b->name;
						});
				}

				const char* categoryOrder[] = { "ui", "sfx", "bgm", "ambient", "other" };
				const float browserHeight = 260.0f;
				const float buttonWidth = 48.0f;
				ImGui::Spacing();
				if (ImGui::BeginChild("##AudioCatalogBrowser", ImVec2(0.0f, browserHeight), true)) {
					for (const char* categoryName : categoryOrder) {
						auto it = audioByCategory.find(categoryName);
						if (it == audioByCategory.end()) {
							continue;
						}

						std::vector<const Audio::AudioAsset*> filteredAssets;
						filteredAssets.reserve(it->second.size());
						for (const auto* asset : it->second) {
							if (!filterLower.empty()) {
								std::string assetLower = asset->name;
								std::transform(assetLower.begin(), assetLower.end(), assetLower.begin(),
									[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
								if (assetLower.find(filterLower) == std::string::npos) {
									continue;
								}
							}

							filteredAssets.push_back(asset);
						}

						if (filteredAssets.empty()) {
							continue;
						}

						if (ImGui::CollapsingHeader((std::string(categoryName) + " (" + std::to_string(filteredAssets.size()) + ")").c_str(),
							filterLower.empty() ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
							if (ImGui::BeginTable((std::string("##AudioCatalogTable_") + categoryName).c_str(), 2,
								ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
								ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 62.0f);
								ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthStretch);

								for (const auto* asset : filteredAssets) {
									ImGui::PushID(asset->name.c_str());
									const bool isPlaying = (currentlyPlaying == asset->name);

									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									if (isPlaying) {
										if (ImGui::Button("Stop", ImVec2(buttonWidth, 0.0f))) {
											if (g_AppState && g_AppState->coreEngine) {
												g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(asset->name);
												currentlyPlaying.clear();
											}
										}
									}
									else {
										if (ImGui::Button("Play", ImVec2(buttonWidth, 0.0f))) {
											if (!currentlyPlaying.empty() && g_AppState && g_AppState->coreEngine) {
												g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(currentlyPlaying);
											}

											if (g_AppState && g_AppState->coreEngine) {
												g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>(
													asset->name,
													asset->volume,
													false);
												currentlyPlaying = asset->name;
											}
										}
									}

									ImGui::TableSetColumnIndex(1);
									ImGui::Selectable(asset->name.c_str(), false);
									if (ImGui::IsItemHovered()) {
										ImGui::SetTooltip("Category: %s\nVolume: %.2f\nLoop: %s\nStream: %s\nPath: %s",
											asset->category.c_str(),
											asset->volume,
											asset->loop ? "Yes" : "No",
											asset->stream ? "Yes" : "No",
											asset->filepath.c_str());
									}

									if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
										ImGui::SetDragDropPayload("AUDIO_ASSET", asset->name.c_str(), asset->name.size() + 1);
										ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Bind: %s", asset->name.c_str());
										ImGui::TextDisabled("Drop on object's audio slot");
										ImGui::EndDragDropSource();
									}

									if (ImGui::BeginPopupContextItem("##AudioCatalogContext")) {
										if (ImGui::MenuItem("Edit Metadata")) {
											originalEditName = asset->name;
											editBuffer = *asset;
											requestOpenEditPopup = true;
										}

										if (ImGui::MenuItem("Remove From Catalog")) {
											pendingRemoveName = asset->name;
											requestOpenRemovePopup = true;
										}

										ImGui::EndPopup();
									}

									ImGui::PopID();
								}

								ImGui::EndTable();
							}
						}
					}
				}

				ImGui::EndChild();

				if (requestOpenEditPopup) {
					ImGui::OpenPopup("Edit Audio Asset");
					requestOpenEditPopup = false;
				}

				if (requestOpenRemovePopup) {
					ImGui::OpenPopup("Confirm Remove Audio");
					requestOpenRemovePopup = false;
				}

				if (ImGui::BeginPopupModal("Edit Audio Asset", nullptr,
					ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
					char nameBuf[128];
					strncpy_s(nameBuf, editBuffer.name.c_str(), sizeof(nameBuf) - 1);
					ImGui::SetNextItemWidth(260.0f);
					if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
						editBuffer.name = nameBuf;
						editBuffer.category = DetectCategoryFromName(editBuffer.name);
					}

					const char* categories[] = { "ui", "sfx", "bgm", "ambient", "other" };
					int selectedCategory = 0;
					for (int i = 0; i < 5; ++i) {
						if (editBuffer.category == categories[i]) {
							selectedCategory = i;
							break;
						}
					}

					ImGui::SetNextItemWidth(180.0f);
					if (ImGui::Combo("Category", &selectedCategory, categories, 5)) {
						editBuffer.category = categories[selectedCategory];
					}

					ImGui::Checkbox("Loop", &editBuffer.loop);
					ImGui::SameLine();
					ImGui::Checkbox("Stream", &editBuffer.stream);
					ImGui::SetNextItemWidth(260.0f);
					ImGui::SliderFloat("Volume", &editBuffer.volume, 0.0f, 1.0f, "%.2f");
					ImGui::TextWrapped("File: %s", editBuffer.filepath.c_str());
					ImGui::Spacing();
					if (ImGui::Button("Save", ImVec2(120.0f, 0.0f))) {
						pendingEditBuffer = editBuffer;
						applyEdit = true;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopupModal("Confirm Remove Audio", nullptr,
					ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
					ImGui::Text("Remove '%s' from catalog?", pendingRemoveName.c_str());
					ImGui::Separator();
					if (ImGui::Button("Remove", ImVec2(120.0f, 0.0f))) {
						applyRemove = true;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
						pendingRemoveName.clear();
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (applyRemove) {
					if (currentlyPlaying == pendingRemoveName && g_AppState && g_AppState->coreEngine) {
						g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(pendingRemoveName);
						currentlyPlaying.clear();
					}

					Audio::AudioCatalog::RemoveAudioAsset(pendingRemoveName);
					pendingRemoveName.clear();
					applyRemove = false;
				}

				if (applyEdit) {
					const bool wasPreviewingEditedAsset = (currentlyPlaying == originalEditName);
					if (wasPreviewingEditedAsset && g_AppState && g_AppState->coreEngine) {
						g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(originalEditName);
						currentlyPlaying.clear();
					}

					if (Audio::AudioCatalog::ReplaceAudioAsset(originalEditName, pendingEditBuffer)) {
						ResourceManager::Instance().UnloadAudio(originalEditName);
						ResourceManager::Instance().LoadAudio(
							pendingEditBuffer.name,
							pendingEditBuffer.filepath,
							pendingEditBuffer.loop,
							pendingEditBuffer.stream
						);
					}
					else {
						TS_LOG_ERROR("[Assets Panel] Failed to update audio asset metadata for '" << originalEditName << "'");
					}

					applyEdit = false;
					originalEditName.clear();
				}

				ImGui::Separator();
			}

			// Lazy-load a generic icon once (placeholder)
			if (!sAudioIcon) {
				sAudioIcon = LoadTextureBypassingCache("../assets/Characters/mc_sprite_front.png");
			}

			bool refreshAudio = false;
			const float iconSize = 32.0f;
			const float audioFileBrowserHeight = 260.0f;
			static char sAudioFileFilter[128] = "";

			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Audio Files (%zu)", sAudio.size());
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputTextWithHint("##AudioFileFilter", "Search source audio files...", sAudioFileFilter, IM_ARRAYSIZE(sAudioFileFilter));
			std::string audioFileFilterLower = sAudioFileFilter;
			std::transform(audioFileFilterLower.begin(), audioFileFilterLower.end(), audioFileFilterLower.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			std::unordered_set<std::string> catalogFilePaths;
			catalogFilePaths.reserve(catalogAssets.size());
			for (const auto& asset : catalogAssets) {
				catalogFilePaths.insert(NormalizeAudioPath(asset.filepath));
			}

			if (ImGui::BeginChild("##AudioFilesBrowser", ImVec2(0.0f, audioFileBrowserHeight), true,
				ImGuiWindowFlags_HorizontalScrollbar)) {
				if (ImGui::BeginTable("##AudioFilesTable", 3,
					ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
					ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 38.0f);
					ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 0.78f);
					ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 68.0f);

					for (const auto& path : sAudio) {
						const std::string displayName = fs::path(path).filename().string();
						const std::string normalizedPath = NormalizeAudioPath(path);
						const bool inCatalog = catalogFilePaths.find(normalizedPath) != catalogFilePaths.end();
						const std::string searchableText = displayName + " " + normalizedPath;

						if (!audioFileFilterLower.empty()) {
							std::string searchableLower = searchableText;
							std::transform(searchableLower.begin(), searchableLower.end(), searchableLower.begin(),
								[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
							if (searchableLower.find(audioFileFilterLower) == std::string::npos) {
								continue;
							}
						}

						ImGui::PushID(path.c_str());
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						if (sAudioIcon) {
							ImTextureID texID = (ImTextureID)(intptr_t)sAudioIcon->GetID();
							ImGui::Image(
								texID,
								ImVec2(iconSize, iconSize),
								ImVec2(0, 1),
								ImVec2(1, 0));
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::Selectable(displayName.c_str(), false, 0, ImVec2(0.0f, iconSize));
						if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
							ImGui::SetTooltip("%s", path.c_str());
						}

						// Double-click to add to catalog if not already there
						if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
							ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
							!inCatalog) {

							Audio::AudioAsset newAsset;
							newAsset.filepath = normalizedPath;

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
							newAsset.category = DetectCategoryFromName(newAsset.name);
							newAsset.volume = 1.0f;

							if (Audio::AudioCatalog::AddAudioAsset(newAsset)) {
								ResourceManager::Instance().LoadAudio(
									newAsset.name,
									newAsset.filepath,
									newAsset.loop,
									newAsset.stream
								);

								const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
								if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
									TS_LOG_INFO("[Assets Panel] Catalog auto-saved after double-click add to: " << catalogPath);
								}
							}
						}

						if (ImGui::BeginDragDropSource()) {
							ImGui::SetDragDropPayload("AUDIO_PATH", path.c_str(), path.size() + 1);
							ImGui::TextUnformatted("Audio");
							ImGui::TextWrapped("%s", path.c_str());
							ImGui::EndDragDropSource();
						}

						if (ImGui::BeginPopupContextItem((std::string("ctx_audio##") + path).c_str())) {
							if (ImGui::MenuItem("Delete File")) {
								if (MoveToTrash(path)) {
									refreshAudio = true;
								}
							}

							ImGui::EndPopup();
						}

						ImGui::TableSetColumnIndex(2);
						if (inCatalog) {
							ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.55f, 1.0f), "Catalog");
						}
						else {
							ImGui::TextDisabled("Source");
						}

						ImGui::PopID();
					}

					ImGui::EndTable();
				}
			}

			ImGui::EndChild();

			if (refreshAudio) {
				QueueAudioRefresh();
			}
		}

		ImGui::EndChild();
		ImGui::End();
		(void)editor;		 // currently unused in this panel; keep parameter for future hooks
		(void)selectedIndex; // currently unused in this panel; keep parameter for future hooks
	}
#else
	/**
	 * @brief Draws assets panel.
	 * @param editor Level editor state to operate on.
	 * @param scene Scene being processed.
	 * @param selectedIndex Hierarchy selection tracked by the editor.
	 * @param selectedObjectId Selected game object identifier.
	 */
	void DrawAssetsPanel(LevelEditor& /*editor*/, Scene& /*scene*/, int& /*selectedIndex*/, int /*selectedObjectId*/) {
		// Editor UI disabled in Release.
	}
#endif
}
