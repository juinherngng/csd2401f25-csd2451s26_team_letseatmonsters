/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAudioControl.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:       Implementation of the Level Editor Audio Control panel.
					- Volume sliders for all audio assets
					- Play/Stop buttons for testing
					- Real-time volume adjustment
					- Master volume control

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <string>
#include <unordered_map>

#include "EngineCore/ApplicationState.hpp"
#include "EngineCore/AudioLoading.hpp"
#include "EngineCore/AudioManager.hpp"
#include "EngineCore/ConfigManager.hpp"
#include "EngineCore/Core.hpp"
#include "EngineCore/FilePaths.hpp"
#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorPanelAudioControl.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/Message.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

#ifdef _DEBUG
#include <imgui.h>
#endif

namespace LEPANELAUDIOCONTROL {
#ifdef _DEBUG
	namespace {
		static std::string ResolveConfigPathForAudioControl() {
			std::string configPath;
			if (ConfigManager::ResolveAssetPath(configPath)) {
				return configPath;
			}

			return FilePaths::JoinPath(FilePaths::Dirs::ASSETS, "config.txt");
		}
	}

	/**
	 * @brief Draws the docked Audio Control panel for previewing and tuning audio.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 */
	void DrawAudioControlPanel(LevelEditor& editor, Scene& scene) {
		// Dock into the main dockspace on first use
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Audio Control###LE_AudioControl")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Audio Playback Control");

		// Track which audio is currently playing
		static std::string currentlyPlaying = "";

		// Volume cache for all audio assets (persistent across frames)
		static std::unordered_map<std::string, float> volumeCache;

		// Track if we've initialized the cache from the catalog
		static bool volumeCacheInitialized = false;

		// Flag to force cache reload (set by Reload Volumes button)
		static bool forceReloadCache = false;

		// Get all audio assets from catalog
		const auto& catalogAssets = Audio::AudioCatalog::GetAllAssets();

		if (catalogAssets.empty()) {
			ImGui::TextDisabled("No audio assets in catalog.");
			ImGui::TextDisabled("Import audio files in the Assets panel.");
			ImGui::End();
			return;
		}

		// Mirror the runtime config in a local mixer state so the sliders act like a dedicated audio console.
		static ConfigManager::Settings mixerSettings = ConfigManager::LoadFromAssetsOrDefaults();
		static bool mixerSettingsInitialized = false;
		if (!mixerSettingsInitialized && g_AppState && g_AppState->coreEngine) {
			if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
				mixerSettings.masterVolume = audioMgr->GetMasterVolume();
				mixerSettings.bgmVolume = audioMgr->GetBgmVolume();
				mixerSettings.vfxVolume = audioMgr->GetVfxVolume();
			}

			ConfigManager::Validate(mixerSettings);
			mixerSettingsInitialized = true;
		}

		auto ApplyMixerSettings = [&]() {
			ConfigManager::Validate(mixerSettings);
			if (g_AppState && g_AppState->coreEngine) {
				if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
					audioMgr->ApplySettings(mixerSettings);
				}
			}
			};

		ImGui::SeparatorText("Mixer");
		ImGui::PushTextWrapPos(0.0f);
		ImGui::TextDisabled("Adjust live runtime levels first, then save when they feel right.");
		ImGui::PopTextWrapPos();
		ImGui::Spacing();

		ImGui::TextUnformatted("Master");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##MasterVolume", &mixerSettings.masterVolume, 0.0f, 1.0f, "%.2f")) {
			ApplyMixerSettings();
		}

		ImGui::TextUnformatted("BGM");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##BgmVolume", &mixerSettings.bgmVolume, 0.0f, 1.0f, "%.2f")) {
			ApplyMixerSettings();
		}

		ImGui::TextUnformatted("SFX");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##VfxVolume", &mixerSettings.vfxVolume, 0.0f, 1.0f, "%.2f")) {
			ApplyMixerSettings();
		}

		ImGui::Spacing();
		const float mixButtonSpacing = ImGui::GetStyle().ItemSpacing.x;
		const float mixAvailWidth = ImGui::GetContentRegionAvail().x;
		const float mixButtonWidth = std::max(120.0f, (mixAvailWidth - mixButtonSpacing) * 0.5f);
		if (ImGui::Button("Save Mixer to Config", ImVec2(mixButtonWidth, 0.0f))) {
			const std::string configPath = ResolveConfigPathForAudioControl();
			if (ConfigManager::Save(configPath, mixerSettings)) {
				TS_LOG_INFO("[Audio Control] Mixer settings saved to config: " << configPath);
				ImGui::OpenPopup("Config Saved");
			}
			else {
				TS_LOG_ERROR("[Audio Control] Failed to save config: " << configPath);
				ImGui::OpenPopup("Config Save Failed");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload Mixer", ImVec2(mixButtonWidth, 0.0f))) {
			mixerSettings = ConfigManager::LoadFromAssetsOrDefaults();
			ConfigManager::Validate(mixerSettings);
			ApplyMixerSettings();
		}

		// Config save success popup
		if (ImGui::BeginPopupModal("Config Saved", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Mixer settings saved to config!");
			ImGui::TextDisabled("Master, BGM, and SFX defaults will match on next startup.");
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Config save error popup
		if (ImGui::BeginPopupModal("Config Save Failed", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to save config!");
			ImGui::TextWrapped("Check console output for details.");
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Save Volume Settings / Reload Volumes row (auto-fit within panel)
		ImGui::Spacing();
		const float actionSpacing = ImGui::GetStyle().ItemSpacing.x;
		const float actionAvailWidth = ImGui::GetContentRegionAvail().x;
		const float actionButtonWidth = std::max(120.0f, (actionAvailWidth - actionSpacing) * 0.5f);
		if (ImGui::Button("Save Volume Settings", ImVec2(actionButtonWidth, 0.0f))) {
			// Persist per-asset volume edits back into the source audio catalog.
			const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
			if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
				TS_LOG_INFO("[Audio Control] Volume settings saved to: " << catalogPath);
				ImGui::OpenPopup("Volume Settings Saved");
			}
			else {
				TS_LOG_ERROR("[Audio Control] Failed to save volume settings!");
				ImGui::OpenPopup("Volume Save Failed");
			}
		}

		ImGui::SameLine();

		// Reload Volumes button
		if (ImGui::Button("Reload Volumes", ImVec2(actionButtonWidth, 0.0f))) {
			// Reload the catalog from disk and rebuild the cache on the next frame.
			const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
			Audio::AudioCatalog::UnloadAllAudio();
			if (Audio::AudioCatalog::LoadCatalogFromFile(catalogPath)) {
				Audio::AudioCatalog::LoadAllAudio();

				// Set flag to force volume cache reload on next frame
				forceReloadCache = true;

				TS_LOG_INFO("[Audio Control] Catalog reloaded. Volume cache will reinitialize.");
				ImGui::OpenPopup("Volumes Reloaded");
			}
			else {
				TS_LOG_ERROR("[Audio Control] Failed to reload catalog!");
				ImGui::OpenPopup("Reload Failed");
			}
		}

		// Reload success popup
		if (ImGui::BeginPopupModal("Volumes Reloaded", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Volume settings reloaded successfully!");
			ImGui::TextDisabled("All volumes restored from saved catalog.");
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Reload failed popup
		if (ImGui::BeginPopupModal("Reload Failed", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to reload catalog!");
			ImGui::TextWrapped("Check console output for details.");
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Audio assets table
		ImGui::SeparatorText("Per-Asset Levels");
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Catalog assets: %zu", catalogAssets.size());
		if (!currentlyPlaying.empty()) {
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextDisabled("Previewing: %s", currentlyPlaying.c_str());
			ImGui::PopTextWrapPos();
		}

		static char sAudioFilter[128] = "";
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputTextWithHint("##AudioControlFilter", "Search by asset name...", sAudioFilter, IM_ARRAYSIZE(sAudioFilter));
		std::string filterLower = sAudioFilter;
		std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		ImGui::Spacing();

		// Begin a child region for scrollable content
		ImGui::BeginChild("##AudioAssetsScroll", ImVec2(0, 0), false);

		// Initialize/update volume cache from catalog on first frame, when catalog changes, or when forced
		// This ensures loaded volumes from file are reflected in the UI
		if (!volumeCacheInitialized || volumeCache.size() != catalogAssets.size() || forceReloadCache) {
			// Mirror catalog values into UI-owned storage so sliders can edit them in-place.
			volumeCache.clear();
			for (const auto& asset : catalogAssets) {
				volumeCache[asset.name] = asset.volume;
			}
			volumeCacheInitialized = true;
			forceReloadCache = false;
			TS_LOG_DEBUG("[Audio Control] Volume cache initialized/updated with " << volumeCache.size() << " entries");
		}

		// Group audio by category
		std::unordered_map<std::string, std::vector<const Audio::AudioAsset*>> audioByCategory;
		for (const auto& asset : catalogAssets) {
			// Group by category first so the panel stays easy to scan as the catalog grows.
			audioByCategory[asset.category].push_back(&asset);
		}

		// Sort audio within each category by name (this groups by prefix like ui_, sfx_, bgm_)
		for (auto& [category, assets] : audioByCategory) {
			std::sort(assets.begin(), assets.end(),
				[](const Audio::AudioAsset* a, const Audio::AudioAsset* b) {
					return a->name < b->name;
				});
		}

		// Display each category
		const char* categoryColors[] = { "ui", "sfx", "bgm", "ambient", "other" };
		const ImVec4 colors[] = {
			ImVec4(0.5f, 0.8f, 1.0f, 1.0f),  // ui - light blue
			ImVec4(1.0f, 0.8f, 0.5f, 1.0f),  // sfx - orange
			ImVec4(0.8f, 0.5f, 1.0f, 1.0f),  // bgm - purple
			ImVec4(0.5f, 1.0f, 0.8f, 1.0f),  // ambient - cyan
			ImVec4(0.8f, 0.8f, 0.8f, 1.0f)   // other - gray
		};

		for (int catIdx = 0; catIdx < 5; ++catIdx) {
			const std::string category = categoryColors[catIdx];

			if (audioByCategory.find(category) == audioByCategory.end() || audioByCategory[category].empty()) {
				continue;
			}

			std::vector<const Audio::AudioAsset*> filteredAssets;
			filteredAssets.reserve(audioByCategory[category].size());
			for (const auto* asset : audioByCategory[category]) {
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

			// Category header
			ImGui::PushStyleColor(ImGuiCol_Text, colors[catIdx]);
			if (ImGui::CollapsingHeader((category + " (" + std::to_string(filteredAssets.size()) + ")").c_str(),
				filterLower.empty() ? 0 : ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PopStyleColor();

				if (ImGui::BeginTable((std::string("##AudioControlTable_") + category).c_str(), 4,
					ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthFixed, 62.0f);
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.55f);
					ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 48.0f);
					ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthStretch, 0.45f);

					for (const auto* asset : filteredAssets) {
						ImGui::PushID(asset->name.c_str());

						bool isPlaying = (currentlyPlaying == asset->name);

						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						if (isPlaying) {
							if (ImGui::Button("Stop", ImVec2(60, 0))) {
								// Stop preview playback through the shared message bus.
								if (g_AppState && g_AppState->coreEngine) {
									g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(asset->name);
									currentlyPlaying = "";
									TS_LOG_INFO("[Audio Control] Stopped: " << asset->name);
								}
							}
						}
						else {
							if (ImGui::Button("Play", ImVec2(60, 0))) {
								// Ensure only one preview plays at a time.
								if (!currentlyPlaying.empty() && g_AppState && g_AppState->coreEngine) {
									g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(currentlyPlaying);
								}

								// Play the selected audio
								if (g_AppState && g_AppState->coreEngine) {
									g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>(
										asset->name,
										asset->volume,
										false
									);
									currentlyPlaying = asset->name;
									TS_LOG_INFO("[Audio Control] Playing: " << asset->name);
								}
							}
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::Selectable(asset->name.c_str(), false);
						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Category: %s\nLoop: %s\nStream: %s\nPath: %s",
								asset->category.c_str(),
								asset->loop ? "Yes" : "No",
								asset->stream ? "Yes" : "No",
								asset->filepath.c_str());
						}

						// Make the asset name a drag source for binding to game objects
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
							ImGui::SetDragDropPayload("AUDIO_ASSET", asset->name.c_str(), asset->name.size() + 1);
							ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Bind: %s", asset->name.c_str());
							ImGui::TextDisabled("Drop on object's audio slot");
							ImGui::EndDragDropSource();
						}

						ImGui::TableSetColumnIndex(2);
						std::string flags;
						if (asset->loop) {
							flags += "L";
						}

						if (asset->stream) {
							if (!flags.empty()) {
								flags += "/";
							}

							flags += "S";
						}

						ImGui::TextDisabled("%s", flags.empty() ? "-" : flags.c_str());

						ImGui::TableSetColumnIndex(3);
						// Volume slider - use the cache that was initialized from catalog
						float& vol = volumeCache[asset->name];
						ImGui::SetNextItemWidth(-FLT_MIN);
						if (ImGui::SliderFloat(("##Volume" + asset->name).c_str(), &vol, 0.0f, 1.0f, "%.2f")) {
							// Update preview playback immediately when the active asset is being adjusted.
							if (isPlaying && g_AppState && g_AppState->coreEngine) {
								if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
									audioMgr->SetVolume(asset->name, vol);
								}
							}

							// Update the catalog asset volume immediately in memory
							// This will be saved when user clicks "Save Volume Settings"
							const_cast<Audio::AudioAsset*>(asset)->volume = vol;
						}

						ImGui::PopID();
					}

					ImGui::EndTable();
				}
			}
			else {
				ImGui::PopStyleColor();
			}
		}

		ImGui::EndChild();

		ImGui::End();

		(void)editor; // Suppress unused parameter warning
		(void)scene;  // Suppress unused parameter warning
	}
#else
	/**
	 * @brief Stub implementation used when the editor UI is compiled out.
	 * @param editor Unused level editor reference.
	 * @param scene Unused scene reference.
	 */
	void DrawAudioControlPanel(LevelEditor& /*editor*/, Scene& /*scene*/) {
		// No-op in Release builds audio control editor disabled.
	}
#endif

} // namespace LEPANELAUDIOCONTROL
