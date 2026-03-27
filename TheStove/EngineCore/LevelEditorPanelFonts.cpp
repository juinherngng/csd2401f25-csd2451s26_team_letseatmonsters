/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelFonts.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (70%)
 CO-AUTHORS:		Vu Phan Hung, phanhung.vu@digipen.edu   (30%)

 DESCRIPTION:       Implementation of the Fonts panel for Level Editor.
					Manages font loading and text object data.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <cstdio> // snprintf
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/FontSystem.hpp"
#include "EngineCore/LevelEditor.hpp"
#include "EngineCore/LevelEditorPanelFonts.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

#ifdef _DEBUG
#include <imgui.h>
#endif

#if defined(_WIN32)
#include <Windows.h>
#endif
namespace {
	void FontLog(const std::string& msg) {
		// 1) Visual Studio Output Window (works in Release when run under debugger)
#if defined(_WIN32)
		OutputDebugStringA((msg + "\n").c_str());
#endif

		// 2) Mirror into the shared engine logger for consistent diagnostics.
		TS_LOG_INFO(msg);

		// 3) Always log to a file next to the exe working directory
		static std::ofstream file("font_debug.log", std::ios::app);
		if (file.is_open()) {
			file << msg << "\n";
			file.flush();
		}
	}

	std::string BoolStr(bool v) {
		return v ? "true" : "false";
	}
}

namespace fs = std::filesystem;
namespace {
	std::string ResolveFontPathAGENCYB() {
		// Try multiple likely paths (debug vs release working dir differences)
		const std::vector<std::string> candidates = {
			FilePaths::Fonts::AGENCYB,
			"assets/Font/AGENCYB.ttf",
			std::string(FilePaths::Dirs::FONTS) + "AGENCYB.ttf"
		};

		for (const auto& p : candidates) {
			if (!p.empty() && fs::exists(p)) {
				return p;
			}
		}
		return {};
	}
}



namespace LEPANELFONTS {
	// =========================
	// Runtime data (Debug + Release)
	// =========================
	static std::vector<TextObjectData> sTextObjects;
	static int sSelectedTextIndex = -1;

	// Runtime font registry (Debug + Release)
	static std::vector<std::string> sRuntimeLoadedFonts;
	static std::vector<std::string> sLoadedFonts;

	bool EnsureFontLoaded(const std::string& fontName,
		const std::string& fontPath,
		unsigned int fontSize) {
		if (fontName.empty() || fontPath.empty() || fontSize == 0)
			return false;

		// If ResourceManager already has it, we're done
		if (ResourceManager::Instance().GetFont(fontName))
			return true;

		FontSystem::Font* font = ResourceManager::Instance().LoadFont(fontName, fontPath, fontSize);
		if (!font) {
			FontLog("[Fonts] EnsureFontLoaded FAILED");
			FontLog("[Fonts]   name = " + fontName);
			FontLog("[Fonts]   path = " + fontPath);
			FontLog("[Fonts]   exists(path) = " + std::string(BoolStr(fs::exists(fontPath))));
			FontLog("[Fonts]   cwd = " + fs::current_path().string());
			return false;
		}
		FontLog("[Fonts] EnsureFontLoaded OK: " + fontName + " (" + fontPath + ")");

		// Track in runtime list (optional)
		if (std::find(sRuntimeLoadedFonts.begin(), sRuntimeLoadedFonts.end(), fontName) == sRuntimeLoadedFonts.end())
			sRuntimeLoadedFonts.push_back(fontName);

#ifdef _DEBUG
		// also mirror into editor list for the dropdown
		if (std::find(sLoadedFonts.begin(), sLoadedFonts.end(), fontName) == sLoadedFonts.end())
			sLoadedFonts.push_back(fontName);
#endif

		return true;
	}

	static bool LoadDefaultFontByName(const std::string& fontName) {
		if (fontName == "font1" || fontName == "font2") {
			const auto cwd = fs::current_path().string();
			const std::string resolved = ResolveFontPathAGENCYB();

			FontLog("[Fonts] LoadDefaultFontByName('" + fontName + "')");
			FontLog("[Fonts]   cwd = " + cwd);
			FontLog("[Fonts]   resolved path = " + (resolved.empty() ? "<EMPTY>" : resolved));
			if (!resolved.empty()) {
				FontLog("[Fonts]   exists(resolved) = " + std::string(BoolStr(fs::exists(resolved))));
			}

			if (resolved.empty()) {
				FontLog("[Fonts]   ERROR: Could not resolve AGENCYB.ttf. Release likely missing assets or wrong working dir.");
				return false;
			}

			return EnsureFontLoaded(fontName, resolved, 48);
		}

		return false;
	}


	void EnsureFontsForTextObjectsLoaded() {
		FontLog("[Fonts] EnsureFontsForTextObjectsLoaded()");
		FontLog("[Fonts]   textObjects = " + std::to_string(sTextObjects.size()));

		for (const auto& obj : sTextObjects) {
			FontLog("[Fonts]   obj '" + obj.name + "' fontName='" + obj.fontName +
				"' alpha=" + std::to_string(obj.colorA));

			if (obj.fontName.empty())
				continue;

			if (!ResourceManager::Instance().GetFont(obj.fontName)) {
				FontLog("[Fonts]   font not loaded yet -> loading '" + obj.fontName + "'");
				LoadDefaultFontByName(obj.fontName);
			}
			else {
				FontLog("[Fonts]   already loaded: " + obj.fontName);
			}
		}
	}




#ifdef _DEBUG
	// =========================
	// Editor-only data
	// =========================

	static std::vector<std::string> ListTTFFiles(const std::string& directory) {
		std::vector<std::string> files;
		if (!fs::exists(directory) || !fs::is_directory(directory)) {
			TS_LOG_WARN("[FontPanel] Directory not found: " << directory);
			return files;
		}

		try {
			for (const auto& entry : fs::directory_iterator(directory)) {
				if (entry.is_regular_file()) {
					std::string ext = entry.path().extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
					if (ext == ".ttf") {
						files.push_back(entry.path().string());
					}
				}
			}

			std::sort(files.begin(), files.end());
			TS_LOG_INFO("[FontPanel] Found " << files.size() << " TTF files in " << directory);
		}
		catch (const std::exception& e) {
			TS_LOG_ERROR("[FontPanel] Error listing TTF files: " << e.what());
		}

		return files;
	}
#endif

	// =========================
	// Public API (Debug + Release)
	// =========================
	const std::vector<TextObjectData>& GetTextObjects() {
		return sTextObjects;
	}

	std::vector<TextObjectData>& GetMutableTextObjects() {
		return sTextObjects;
	}

	void SetTextObjects(const std::vector<TextObjectData>& textObjects) {
		sTextObjects = textObjects;
		sSelectedTextIndex = -1;

		EnsureFontsForTextObjectsLoaded();

#ifdef _DEBUG
		// Track fonts used by text objects (editor convenience)
		for (const auto& textObj : textObjects) {
			if (!textObj.fontName.empty()) {
				auto it = std::find(sLoadedFonts.begin(), sLoadedFonts.end(), textObj.fontName);
				if (it == sLoadedFonts.end()) {
					if (ResourceManager::Instance().GetFont(textObj.fontName)) {
						sLoadedFonts.push_back(textObj.fontName);
					}
				}
			}
		}
#endif
	}

	void SetTextObjectsWithScene(const std::vector<TextObjectData>& textObjects, Scene& scene) {
		sTextObjects = textObjects;
		sSelectedTextIndex = -1;

		EnsureFontsForTextObjectsLoaded();
		scene.SetRuntimeTextObjects(textObjects);

#ifdef _DEBUG
		// Track fonts for editor UI
		for (const auto& textObj : textObjects) {
			if (!textObj.fontName.empty()) {
				auto it = std::find(sLoadedFonts.begin(), sLoadedFonts.end(), textObj.fontName);
				if (it == sLoadedFonts.end()) {
					if (ResourceManager::Instance().GetFont(textObj.fontName)) {
						sLoadedFonts.push_back(textObj.fontName);
					}
				}
			}
		}

		TS_LOG_INFO("[FontPanel] Loaded " << textObjects.size()
			<< " text objects with scene layer registration");
#endif
	}

	bool SetTextByName(const std::string& name, const std::string& newText) {
		if (name.empty())
			return false;

		for (auto& obj : sTextObjects) {
			if (obj.name == name) {
				obj.text = newText;
				return true;
			}
		}

#ifdef _DEBUG
		TS_LOG_WARN("[LEPANELFONTS] SetTextByName failed: '" << name << "' not found");
#endif
		return false;
	}

	void ClearTextObjects() {
		sTextObjects.clear();
		sSelectedTextIndex = -1;
	}

	int GetSelectedTextIndex() {
		return sSelectedTextIndex;
	}

	void SetSelectedTextIndex(int index) {
		sSelectedTextIndex = index;
	}

	const std::vector<std::string>& GetLoadedFontNames() {
		return sRuntimeLoadedFonts; // works in both builds
	}

	void DrawFontsPanel(LevelEditor& editor, Scene& scene) {
#ifdef _DEBUG
		// Mirror the editor cache into the live scene every frame so text preview stays runtime-driven.
		scene.SetRuntimeTextObjects(sTextObjects);

		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Fonts###LE_Fonts")) {
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("Font Management");

		static char fontPathBuf[256] = {};
		static bool fontPathInitialized = false;
		if (!fontPathInitialized) {
			std::snprintf(fontPathBuf, sizeof(fontPathBuf), "%s", FilePaths::Fonts::AGENCYB);
			fontPathInitialized = true;
		}
		static int  fontSizeBuf = 48;
		static char fontNameBuf[64] = "font1";

		ImGui::TextUnformatted("Load Font");

		static std::vector<std::string> sFontFiles;
		static bool sFirstTime = true;

		if (sFirstTime) {
			std::vector<std::string> paths = { FilePaths::Dirs::FONTS, "assets/Font" };
			for (const auto& path : paths) {
				auto files = ListTTFFiles(path);
				sFontFiles.insert(sFontFiles.end(), files.begin(), files.end());
			}
			sFirstTime = false;
		}

		if (ImGui::Button("Refresh##fonts")) {
			sFontFiles.clear();
			std::vector<std::string> paths = { FilePaths::Dirs::FONTS, "assets/Font" };
			for (const auto& path : paths) {
				auto files = ListTTFFiles(path);
				sFontFiles.insert(sFontFiles.end(), files.begin(), files.end());
			}
		}

		ImGui::SameLine();
		ImGui::TextDisabled("(%zu fonts found)", sFontFiles.size());

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##FontFileCombo", fontPathBuf)) {
			for (size_t i = 0; i < sFontFiles.size(); ++i) {
				const bool isSelected = (sFontFiles[i] == std::string(fontPathBuf));
				if (ImGui::Selectable(sFontFiles[i].c_str(), isSelected)) {
					std::snprintf(fontPathBuf, sizeof(fontPathBuf), "%s", sFontFiles[i].c_str());
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputText("##FontPath", fontPathBuf, IM_ARRAYSIZE(fontPathBuf));

		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 150.0f);

		ImGui::TextUnformatted("Font Name");
		ImGui::NextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##FontName", fontNameBuf, IM_ARRAYSIZE(fontNameBuf));
		ImGui::NextColumn();

		ImGui::TextUnformatted("Font Size");
		ImGui::NextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputInt("##FontSize", &fontSizeBuf);
		ImGui::NextColumn();

		ImGui::Columns(1);

		if (ImGui::Button("Load Font", ImVec2(120, 0))) {
			std::string fontName(fontNameBuf);
			std::string fontPath(fontPathBuf);
			unsigned int fontSize = static_cast<unsigned int>(fontSizeBuf);

			if (!fontName.empty() && !fontPath.empty() && fontSize > 0) {
				if (EnsureFontLoaded(fontName, fontPath, fontSize)) {
					auto it = std::find(sLoadedFonts.begin(), sLoadedFonts.end(), fontName);
					if (it == sLoadedFonts.end()) {
						sLoadedFonts.push_back(fontName);
					}
				}
			}
		}

		ImGui::Separator();
		ImGui::SeparatorText("Loaded Fonts");

		if (sLoadedFonts.empty()) {
			ImGui::TextDisabled("No fonts loaded yet.");
		}
		else {
			for (const auto& loadedName : sLoadedFonts) {
				FontSystem::Font* loadedFont = ResourceManager::Instance().GetFont(loadedName);
				if (loadedFont) {
					ImGui::BulletText("%s (size: %u)", loadedName.c_str(), loadedFont->GetFontSize());
				}
			}
		}

		ImGui::Separator();
		ImGui::SeparatorText("Text Objects");

		if (ImGui::Button("Create Text Object")) {
			TextObjectData newText;
			newText.name = "Text " + std::to_string(sTextObjects.size() + 1);
			newText.fontName = sLoadedFonts.empty() ? "" : sLoadedFonts[0];
			newText.text = "Sample Text";
			newText.x = 100.0f;
			newText.y = 100.0f;

			sTextObjects.push_back(newText);
			sSelectedTextIndex = static_cast<int>(sTextObjects.size()) - 1;
		}

		ImGui::SameLine();

		if (ImGui::Button("Delete Selected") && sSelectedTextIndex >= 0 && sSelectedTextIndex < static_cast<int>(sTextObjects.size())) {
			sTextObjects.erase(sTextObjects.begin() + sSelectedTextIndex);
			sSelectedTextIndex = -1;
		}

		if (ImGui::BeginListBox("##TextObjects", ImVec2(-FLT_MIN, 150.0f))) {
			for (int i = 0; i < static_cast<int>(sTextObjects.size()); ++i) {
				const bool isSelected = (sSelectedTextIndex == i);
				std::string label = sTextObjects[i].name + " [" + sTextObjects[i].fontName + "]";
				if (ImGui::Selectable(label.c_str(), isSelected)) {
					sSelectedTextIndex = i;
				}
			}
			ImGui::EndListBox();
		}

		if (sSelectedTextIndex >= 0 && sSelectedTextIndex < static_cast<int>(sTextObjects.size())) {
			ImGui::Separator();
			ImGui::SeparatorText("Text Properties");

			TextObjectData& textObj = sTextObjects[sSelectedTextIndex];

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 150.0f);

			ImGui::TextUnformatted("Name");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			char nameBuf[64];
			std::snprintf(nameBuf, sizeof(nameBuf), "%s", textObj.name.c_str());
			if (ImGui::InputText("##Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
				textObj.name = nameBuf;
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Font");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##Font", textObj.fontName.c_str())) {
				for (const auto& loadedName : sLoadedFonts) {
					const bool isSelected = (textObj.fontName == loadedName);
					if (ImGui::Selectable(loadedName.c_str(), isSelected)) {
						textObj.fontName = loadedName;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Text");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			char textBuf[256];
			std::snprintf(textBuf, sizeof(textBuf), "%s", textObj.text.c_str());
			if (ImGui::InputText("##Text", textBuf, IM_ARRAYSIZE(textBuf))) {
				textObj.text = textBuf;
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Position X");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##PosX", &textObj.x, 1.0f);
			ImGui::NextColumn();

			ImGui::TextUnformatted("Position Y");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##PosY", &textObj.y, 1.0f);
			ImGui::NextColumn();

			ImGui::TextUnformatted("Scale");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##Scale", &textObj.scale, 0.01f, 0.1f, 10.0f);
			ImGui::NextColumn();

			ImGui::TextUnformatted("Rotation");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::SliderFloat("##Rotation", &textObj.rotation, 0.0f, 360.0f, "%.1f deg");
			ImGui::NextColumn();

			ImGui::TextUnformatted("Rotation Mode");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			const char* rotModeItems[] = { "Block (Normal)", "Per-Character (Curved)" };
			int currentMode = textObj.useBlockRotation ? 0 : 1;
			if (ImGui::Combo("##RotMode", &currentMode, rotModeItems, IM_ARRAYSIZE(rotModeItems))) {
				textObj.useBlockRotation = (currentMode == 0);
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Color");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			float color[4] = { textObj.colorR, textObj.colorG, textObj.colorB, textObj.colorA };
			if (ImGui::ColorEdit4("##Color", color)) {
				textObj.colorR = color[0];
				textObj.colorG = color[1];
				textObj.colorB = color[2];
				textObj.colorA = color[3];
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Layer");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			char layerBuf[64];
			std::snprintf(layerBuf, sizeof(layerBuf), "%s", textObj.layer.c_str());
			if (ImGui::InputText("##Layer", layerBuf, IM_ARRAYSIZE(layerBuf))) {
				textObj.layer = layerBuf;
				scene.AddLayer(textObj.layer);
			}
			ImGui::NextColumn();

			ImGui::TextUnformatted("Visible");
			ImGui::NextColumn();
			ImGui::Checkbox("##Visible", &textObj.visible);
			ImGui::NextColumn();

			ImGui::TextUnformatted("Depth");
			ImGui::NextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##Depth", &textObj.depth, 0.01f);
			ImGui::NextColumn();

			ImGui::Columns(1);
		}

		ImGui::End();
		(void)editor;
		(void)scene;
#else
		(void)editor;
		(void)scene;
#endif
	}
}
