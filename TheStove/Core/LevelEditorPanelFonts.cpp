/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelFonts.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:       Implementation of the Fonts panel for Level Editor.
                    Manages font loading and text object data.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#ifdef _DEBUG
#include <imgui.h>
#endif

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstdio> // snprintf

#include "LevelEditorPanelFonts.hpp"
#include "LevelEditor.hpp"
#include "FontSystem.hpp"
#include "FilePaths.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/SceneManager.hpp"

#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <Windows.h>
#endif
namespace {
    void FontLog(const std::string& msg)
    {
        // 1) Visual Studio Output Window (works in Release when run under debugger)
#if defined(_WIN32)
        OutputDebugStringA((msg + "\n").c_str());
#endif

        // 2) Also try stderr (works if you have a console attached)
        std::cerr << msg << std::endl;

        // 3) Always log to a file next to the exe working directory
        static std::ofstream file("font_debug.log", std::ios::app);
        if (file.is_open()) {
            file << msg << "\n";
            file.flush();
        }
    }

    std::string BoolStr(bool v) { return v ? "true" : "false"; }
}

namespace fs = std::filesystem;
namespace {
    std::string ResolveFontPathAGENCYB()
    {
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



namespace LEPANELFONTS
{
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
        unsigned int fontSize)
    {
        if (fontName.empty() || fontPath.empty() || fontSize == 0)
            return false;

        // If ResourceManager already has it, we're done
        if (ResourceManager::Instance().GetFont(fontName))
            return true;

        FontSystem::Font* font = ResourceManager::Instance().LoadFont(fontName, fontPath, fontSize);
        if (!font)
        {
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

    static bool LoadDefaultFontByName(const std::string& fontName)
    {
        if (fontName == "font1" || fontName == "font2")
        {
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


    void EnsureFontsForTextObjectsLoaded()
    {
        FontLog("[Fonts] EnsureFontsForTextObjectsLoaded()");
        FontLog("[Fonts]   textObjects = " + std::to_string(sTextObjects.size()));

        for (const auto& obj : sTextObjects)
        {
            FontLog("[Fonts]   obj '" + obj.name + "' fontName='" + obj.fontName +
                "' alpha=" + std::to_string(obj.colorA));

            if (obj.fontName.empty())
                continue;

            if (!ResourceManager::Instance().GetFont(obj.fontName))
            {
                FontLog("[Fonts]   font not loaded yet -> loading '" + obj.fontName + "'");
                LoadDefaultFontByName(obj.fontName);
            }
            else
            {
                FontLog("[Fonts]   already loaded: " + obj.fontName);
            }
        }
    }




#ifdef _DEBUG
    // =========================
    // Editor-only data
    // =========================

    static std::vector<std::string> ListTTFFiles(const std::string& directory)
    {
        std::vector<std::string> files;
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            std::cerr << "[FontPanel] Directory not found: " << directory << std::endl;
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
            std::cout << "[FontPanel] Found " << files.size() << " TTF files in " << directory << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[FontPanel] Error listing TTF files: " << e.what() << std::endl;
        }

        return files;
    }
#endif

    // =========================
    // Public API (Debug + Release)
    // =========================
    const std::vector<TextObjectData>& GetTextObjects()
    {
        return sTextObjects;
    }

    std::vector<TextObjectData>& GetMutableTextObjects()
    {
        return sTextObjects;
    }

    void SetTextObjects(const std::vector<TextObjectData>& textObjects)
    {
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

    void SetTextObjectsWithScene(const std::vector<TextObjectData>& textObjects, Scene& scene)
    {
        sTextObjects = textObjects;
        sSelectedTextIndex = -1;

        // Register layers (works in both builds)
        for (const auto& textObj : textObjects) {
            if (!textObj.layer.empty()) {
                scene.AddLayer(textObj.layer);
            }
        }

        EnsureFontsForTextObjectsLoaded();

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

        std::cout << "[FontPanel] Loaded " << textObjects.size()
            << " text objects with scene layer registration\n";
#endif
    }

    bool SetTextByName(const std::string& name, const std::string& newText)
    {
        if (name.empty())
            return false;

        for (auto& obj : sTextObjects) {
            if (obj.name == name) {
                obj.text = newText;
                return true;
            }
        }

#ifdef _DEBUG
        std::cerr << "[LEPANELFONTS] SetTextByName failed: '" << name << "' not found\n";
#endif
        return false;
    }

    void ClearTextObjects()
    {
        sTextObjects.clear();
        sSelectedTextIndex = -1;
    }

    int GetSelectedTextIndex()
    {
        return sSelectedTextIndex;
    }

    void SetSelectedTextIndex(int index)
    {
        sSelectedTextIndex = index;
    }

    const std::vector<std::string>& GetLoadedFontNames()
    {
        return sRuntimeLoadedFonts; // works in both builds
    }

    void DrawFontsPanel(LevelEditor& editor, Scene& scene)
    {
#ifdef _DEBUG
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


        // --- keep the rest of your ImGui text-object UI exactly as you already had ---
        // (create/delete/list/edit etc.)
        // IMPORTANT: do NOT re-define the data functions again inside _DEBUG.

        ImGui::End();
        (void)editor;
        (void)scene;
#else
        (void)editor;
        (void)scene;
#endif
    }
}
