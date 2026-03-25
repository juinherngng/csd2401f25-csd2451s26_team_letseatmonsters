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

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"

#include "AudioLoading.hpp"
#include "AudioManager.hpp"
#include "ConfigManager.hpp"
#include "Core.hpp"
#include "LevelEditor.hpp"
#include "LevelEditorPanelAudioControl.hpp"
#include "Message.hpp"

#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

 // Forward declaration and external declaration for ApplicationState from Main.cpp
namespace CoreFramework {
	class CoreEngine;
}

struct ApplicationState {
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
};

extern ApplicationState* g_AppState;

namespace LEPANELAUDIOCONTROL {
#ifdef _DEBUG
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

		// Master volume control
		static float masterVolume = 1.0f;

		// Initialize master volume from AudioManager on first frame
		static bool masterVolumeInitialized = false;
		if (!masterVolumeInitialized && g_AppState && g_AppState->coreEngine) {
			if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
				masterVolume = audioMgr->GetMasterVolume();
				masterVolumeInitialized = true;
			}
		}

		ImGui::Text("Master Volume");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("##MasterVolume", &masterVolume, 0.0f, 1.0f, "%.2f")) {
			// Apply master volume to audio manager
			if (g_AppState && g_AppState->coreEngine) {
				if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
					audioMgr->SetMasterVolume(masterVolume);
				}
			}
		}

		ImGui::SameLine();

		// Save master volume to config file
		const float smallButtonWidth = std::max(110.0f, ImGui::GetContentRegionAvail().x);
		if (ImGui::Button("Save to Config", ImVec2(smallButtonWidth, 0.0f))) {
			// Load current config, update master volume, and save
			ConfigManager::Settings settings = ConfigManager::LoadFromAssetsOrDefaults();
			settings.masterVolume = masterVolume;

			// Save to SOURCE config file (../../assets from build/Release)
			const std::string configPath = "../../assets/config.txt";
			if (ConfigManager::Save(configPath, settings)) {
				std::cout << "[Audio Control] Master volume saved to config: " << masterVolume << std::endl;
				ImGui::OpenPopup("Config Saved");
			}
			else {
				std::cerr << "[Audio Control] ERROR: Failed to save config!" << std::endl;
				ImGui::OpenPopup("Config Save Failed");
			}
		}

		// Config save success popup
		if (ImGui::BeginPopupModal("Config Saved", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Master volume saved to config!");
			ImGui::TextDisabled("This will be the default volume on next startup.");
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
			// Save the catalog with updated volume values to SOURCE directory
			const std::string catalogPath = "../../assets/Audio/AudioCatalog.json";
			if (Audio::AudioCatalog::SaveCatalogToFile(catalogPath)) {
				std::cout << "[Audio Control] Volume settings saved to: " << catalogPath << std::endl;
				ImGui::OpenPopup("Volume Settings Saved");
			}
			else {
				std::cerr << "[Audio Control] ERROR: Failed to save volume settings!" << std::endl;
				ImGui::OpenPopup("Volume Save Failed");
			}
		}

		ImGui::SameLine();

		// Reload Volumes button
		if (ImGui::Button("Reload Volumes", ImVec2(actionButtonWidth, 0.0f))) {
			// Reload catalog from file
			const std::string catalogPath = "../../assets/Audio/AudioCatalog.json";
			Audio::AudioCatalog::UnloadAllAudio();
			if (Audio::AudioCatalog::LoadCatalogFromFile(catalogPath)) {
				Audio::AudioCatalog::LoadAllAudio();

				// Set flag to force volume cache reload on next frame
				forceReloadCache = true;

				std::cout << "[Audio Control] Catalog reloaded. Volume cache will reinitialize." << std::endl;
				ImGui::OpenPopup("Volumes Reloaded");
			}
			else {
				std::cerr << "[Audio Control] ERROR: Failed to reload catalog!" << std::endl;
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
		ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Audio Assets (%zu):", catalogAssets.size());
		ImGui::Spacing();

		// Begin a child region for scrollable content
		ImGui::BeginChild("##AudioAssetsScroll", ImVec2(0, 0), false);

		// Initialize/update volume cache from catalog on first frame, when catalog changes, or when forced
		// This ensures loaded volumes from file are reflected in the UI
		if (!volumeCacheInitialized || volumeCache.size() != catalogAssets.size() || forceReloadCache) {
			volumeCache.clear();
			for (const auto& asset : catalogAssets) {
				volumeCache[asset.name] = asset.volume;
			}
			volumeCacheInitialized = true;
			forceReloadCache = false;
			std::cout << "[Audio Control] Volume cache initialized/updated with " << volumeCache.size() << " entries" << std::endl;
		}

		// Group audio by category
		std::unordered_map<std::string, std::vector<const Audio::AudioAsset*>> audioByCategory;
		for (const auto& asset : catalogAssets) {
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

			// Category header
			ImGui::PushStyleColor(ImGuiCol_Text, colors[catIdx]);
			if (ImGui::CollapsingHeader((category + " (" + std::to_string(audioByCategory[category].size()) + ")").c_str(),
				ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::PopStyleColor();

				for (const auto* asset : audioByCategory[category]) {
					ImGui::PushID(asset->name.c_str());

					bool isPlaying = (currentlyPlaying == asset->name);

					// Asset name (make it draggable)
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "%s", asset->name.c_str());

					// Make the asset name a drag source for binding to game objects
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						// Store the asset name as payload
						ImGui::SetDragDropPayload("AUDIO_ASSET", asset->name.c_str(), asset->name.size() + 1);

						// Show preview while dragging
						ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Bind: %s", asset->name.c_str());
						ImGui::TextDisabled("Drop on object's audio slot");

						ImGui::EndDragDropSource();
					}

					// Play/Stop button
					if (isPlaying) {
						if (ImGui::Button("Stop", ImVec2(60, 0))) {
							if (g_AppState && g_AppState->coreEngine) {
								g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>(asset->name);
								currentlyPlaying = "";
								std::cout << "[Audio Control] Stopped: " << asset->name << std::endl;
							}
						}
					}
					else {
						if (ImGui::Button("Play", ImVec2(60, 0))) {
							// Stop any currently playing preview first
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
								std::cout << "[Audio Control] Playing: " << asset->name << std::endl;
							}
						}
					}

					ImGui::SameLine();

					// Volume slider - use the cache that was initialized from catalog
					float& vol = volumeCache[asset->name];
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::SliderFloat(("##Volume" + asset->name).c_str(), &vol, 0.0f, 1.0f, "%.2f")) {
						// Update volume in real-time if playing
						if (isPlaying && g_AppState && g_AppState->coreEngine) {
							if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
								audioMgr->SetVolume(asset->name, vol);
							}
						}

						// Update the catalog asset volume immediately in memory
						// This will be saved when user clicks "Save Volume Settings"
						const_cast<Audio::AudioAsset*>(asset)->volume = vol;
					}

					// Show audio properties inline
					ImGui::Indent(20.0f);
					ImGui::TextDisabled("Loop: %s | Stream: %s",
						asset->loop ? "Yes" : "No",
						asset->stream ? "Yes" : "No");
					ImGui::Unindent(20.0f);

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::PopID();
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
	void DrawAudioControlPanel(LevelEditor& /*editor*/, Scene& /*scene*/) {
		// No-op in Release builds audio control editor disabled.
	}
#endif

} // namespace LEPANELAUDIOCONTROL

