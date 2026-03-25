/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelConfig.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implementation of the Level Editor Config panel.
					- Supports editing config.txt settings in the editor
					- Applies audio settings live through AudioManager
					- Persists window/audio defaults for the next startup

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Graphics/GraphicsEngine.hpp"

#include "AudioManager.hpp"
#include "ConfigManager.hpp"
#include "Core.hpp"
#include "LevelEditor.hpp"
#include "LevelEditorPanelConfig.hpp"

#include <cfloat>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#ifdef _DEBUG
#include <imgui.h>
#endif

namespace CoreFramework {
	class CoreEngine;
}

struct ApplicationState {
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
};

extern ApplicationState* g_AppState;

namespace {
	/**
	 * @brief Resolves the config file path using the same candidate search order as ConfigManager.
	 * @return Path to the config file used by the editor panel.
	 */
	std::string ResolveConfigPath() {
		std::string resolvedPath;
		if (ConfigManager::ResolveAssetPath(resolvedPath)) {
			return resolvedPath;
		}

		namespace fs = std::filesystem;
		const fs::path cwd = fs::current_path();
		return (cwd / "assets/config.txt").lexically_normal().string();
	}

	/**
	 * @brief Builds the list of config file targets to keep in sync when saving.
	 * @param primaryPath Primary config path currently in use.
	 * @return Ordered list of distinct config save targets.
	 */
	std::vector<std::string> CollectConfigSaveTargets(const std::string& primaryPath) {
		namespace fs = std::filesystem;
		const fs::path cwd = fs::current_path();
		std::vector<fs::path> candidates;
		candidates.emplace_back(primaryPath);
		candidates.emplace_back(cwd / "../../assets/config.txt");
		candidates.emplace_back(cwd / "../assets/config.txt");
		candidates.emplace_back(cwd / "assets/config.txt");
		candidates.emplace_back(cwd / "config.txt");

		std::set<std::string> seen;
		std::vector<std::string> targets;
		for (const auto& candidate : candidates) {
			const std::string normalized = candidate.lexically_normal().string();
			if (seen.insert(normalized).second) {
				targets.push_back(normalized);
			}
		}

		return targets;
	}
}

namespace LEPANELCONFIG {
#ifdef _DEBUG
	/**
	 * @brief Draws the Config panel for viewing, editing, and saving engine settings.
	 * @param editor Shared level editor controller.
	 * @param scene Scene currently being edited.
	 */
	void DrawConfigPanel(LevelEditor& editor, Scene& scene) {
		(void)editor;
		(void)scene;

		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Config###LE_Config")) {
			ImGui::End();
			return;
		}

		static bool loaded = false;
		static std::string configPath;
		static ConfigManager::Settings settings{};

		if (!loaded) {
			configPath = ResolveConfigPath();
			settings = ConfigManager::LoadFromAssetsOrDefaults();
			ConfigManager::Validate(settings);
			loaded = true;
		}

		ImGui::SeparatorText("Config File");
		ImGui::TextWrapped("Path: %s", configPath.c_str());

		if (ImGui::Button("Reload From File")) {
			configPath = ResolveConfigPath();
			ConfigManager::Settings reloaded = settings;
			if (ConfigManager::Load(configPath, reloaded)) {
				settings = reloaded;
				ConfigManager::Validate(settings);
				ImGui::OpenPopup("Config Reloaded");
			}
			else {
				ImGui::OpenPopup("Config Reload Failed");
			}
		}

		if (ImGui::BeginPopupModal("Config Reloaded", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Config reloaded successfully.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Config Reload Failed", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to load config file.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::SeparatorText("Display");

		int width = settings.resolution.width;
		int height = settings.resolution.height;
		ImGui::TextUnformatted("Resolution");
		ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 36.0f) * 0.5f);
		if (ImGui::InputInt("##WindowWidth", &width)) {
			settings.resolution.width = width;
		}

		ImGui::SameLine();
		ImGui::TextUnformatted("x");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::InputInt("##WindowHeight", &height)) {
			settings.resolution.height = height;
		}

		ImGui::Checkbox("Fullscreen", &settings.fullscreen);

		ImGui::SeparatorText("Audio");
		ImGui::SliderFloat("Master Volume", &settings.masterVolume, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("BGM Volume", &settings.bgmVolume, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("VFX Volume", &settings.vfxVolume, 0.0f, 1.0f, "%.2f");

		ConfigManager::Validate(settings);

		if (ImGui::Button("Save Config", ImVec2(140, 0))) {
			configPath = ResolveConfigPath();
			const std::vector<std::string> saveTargets = CollectConfigSaveTargets(configPath);
			bool savedPrimary = false;
			for (const auto& targetPath : saveTargets) {
				const bool saveOk = ConfigManager::Save(targetPath, settings);
				if (targetPath == configPath) {
					savedPrimary = saveOk;
				}
			}

			if (savedPrimary) {
				ImGui::OpenPopup("Config Saved");
			}
			else {
				ImGui::OpenPopup("Config Save Failed");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Apply Audio", ImVec2(140, 0))) {
			if (g_AppState && g_AppState->coreEngine) {
				if (auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>()) {
					audioMgr->ApplySettings(settings);
					ImGui::OpenPopup("Audio Applied");
				}
				else {
					ImGui::OpenPopup("Audio Apply Failed");
				}
			}
			else {
				ImGui::OpenPopup("Audio Apply Failed");
			}
		}

		ImGui::TextDisabled("Display changes are saved for next startup.");

		if (ImGui::BeginPopupModal("Audio Applied", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Applied audio settings at runtime.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Audio Apply Failed", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Audio manager unavailable.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Config Saved", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::Text("Config saved successfully.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Config Save Failed", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to save config file.");
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}
#else
	void DrawConfigPanel(LevelEditor& editor, Scene& scene) {
		(void)editor;
		(void)scene;
	}
#endif
}
