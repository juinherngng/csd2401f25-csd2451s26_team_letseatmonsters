/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelFonts.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:       Implementation of the Fonts panel for Level Editor.
                    Manages font loading and text object data (UI only).

        All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

#include "LevelEditorPanelFonts.hpp"
#include "LevelEditor.hpp"
#include "FontSystem.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/SceneManager.hpp"

namespace fs = std::filesystem;

namespace LEPANELFONTS {

#ifdef _DEBUG
    // Static storage for fonts and text object data (UI state only)
    static std::vector<std::string> sLoadedFonts;
    static std::vector<TextObjectData> sTextObjects;
    static int sSelectedTextIndex = -1;

    // Helper function to list TTF files in a directory
    static std::vector<std::string> ListTTFFiles(const std::string& directory) {
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
        } catch (const std::exception& e) {
            std::cerr << "[FontPanel] Error listing TTF files: " << e.what() << std::endl;
        }
        
        return files;
    }

    const std::vector<TextObjectData>& GetTextObjects() {
        return sTextObjects;
    }
    
    // Functions for level serialization integration
    void SetTextObjects(const std::vector<TextObjectData>& textObjects) {
        sTextObjects = textObjects;
        sSelectedTextIndex = -1;
        
        // Also ensure fonts used by text objects are tracked
        for (const auto& textObj : textObjects) {
            if (!textObj.fontName.empty()) {
                auto it = std::find(sLoadedFonts.begin(), sLoadedFonts.end(), textObj.fontName);
                if (it == sLoadedFonts.end()) {
                    // Check if font is actually loaded in ResourceManager
                    if (ResourceManager::Instance().GetFont(textObj.fontName)) {
                        sLoadedFonts.push_back(textObj.fontName);
                    }
                }
            }
        }
    }
    
    void ClearTextObjects() {
        sTextObjects.clear();
        sSelectedTextIndex = -1;
    }
    
    std::vector<TextObjectData>& GetMutableTextObjects() {
        return sTextObjects;
    }

    void DrawFontsPanel(LevelEditor& editor, Scene& scene) {
        ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Fonts###LE_Fonts")) {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Font Management");

        // Font loading section
        static char fontPathBuf[256] = "../assets/Font/ChrustyRock-ORLA.ttf";
        static int fontSizeBuf = 48;
        static char fontNameBuf[64] = "font1";

        ImGui::TextUnformatted("Load Font");
        
        // List available TTF files
        static std::vector<std::string> sFontFiles;
        static bool sFirstTime = true;
        
        if (sFirstTime) {
            std::vector<std::string> paths = {"../assets/Font", "assets/Font"};
            for (const auto& path : paths) {
                auto files = ListTTFFiles(path);
                sFontFiles.insert(sFontFiles.end(), files.begin(), files.end());
            }
            sFirstTime = false;
        }
        
        if (ImGui::Button("Refresh##fonts")) {
            sFontFiles.clear();
            std::vector<std::string> paths = {"../assets/Font", "assets/Font"};
            for (const auto& path : paths) {
                auto files = ListTTFFiles(path);
                sFontFiles.insert(sFontFiles.end(), files.begin(), files.end());
            }
        }
        
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu fonts found)", sFontFiles.size());

        // Font file dropdown
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

        // Font path input
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##FontPath", fontPathBuf, IM_ARRAYSIZE(fontPathBuf));

        // Font name and size
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

        // Load Font button
        if (ImGui::Button("Load Font", ImVec2(120, 0))) {
            std::string fontName(fontNameBuf);
            std::string fontPath(fontPathBuf);
            unsigned int fontSize = static_cast<unsigned int>(fontSizeBuf);
            
            if (!fontName.empty() && !fontPath.empty() && fontSize > 0) {
                std::cout << "[FontPanel] Attempting to load font: " << fontName << " from " << fontPath << std::endl;
                
                FontSystem::Font* font = ResourceManager::Instance().LoadFont(fontName, fontPath, fontSize);
                if (font) {
                    auto it = std::find(sLoadedFonts.begin(), sLoadedFonts.end(), fontName);
                    if (it == sLoadedFonts.end()) {
                        sLoadedFonts.push_back(fontName);
                        std::cout << "[FontPanel] Successfully loaded font: " << fontName << std::endl;
                    } else {
                        std::cout << "[FontPanel] Font already loaded: " << fontName << std::endl;
                    }
                } else {
                    std::cerr << "[FontPanel] Failed to load font: " << fontPath << std::endl;
                }
            }
        }

        ImGui::Separator();
        ImGui::SeparatorText("Loaded Fonts");

        // Display loaded fonts
        if (sLoadedFonts.empty()) {
            ImGui::TextDisabled("No fonts loaded yet.");
            ImGui::TextDisabled("Try loading: font1 and font2 with different fonts");
        } else {
            for (const auto& fontName : sLoadedFonts) {
                FontSystem::Font* font = ResourceManager::Instance().GetFont(fontName);
                if (font) {
                    ImGui::BulletText("%s (size: %u)", fontName.c_str(), font->GetFontSize());
                }
            }
        }

        ImGui::Separator();
        ImGui::SeparatorText("Text Objects");

        // Create new text object
        if (ImGui::Button("Create Text Object")) {
            TextObjectData newText;
            newText.name = "Text " + std::to_string(sTextObjects.size() + 1);
            newText.fontName = sLoadedFonts.empty() ? "" : sLoadedFonts[0];
            newText.text = "Sample Text";
            newText.x = 100.0f;
            newText.y = 100.0f;
            newText.scale = 1.0f;
            newText.rotation = 0.0f;
            newText.useBlockRotation = true;  // Default to block rotation
            newText.colorR = 1.0f;
            newText.colorG = 1.0f;
            newText.colorB = 1.0f;
            newText.colorA = 1.0f;
            newText.layer = "1";  // Default layer
            
            sTextObjects.push_back(newText);
            sSelectedTextIndex = static_cast<int>(sTextObjects.size()) - 1;
        }

        ImGui::SameLine();

        // Delete selected text object
        if (ImGui::Button("Delete Selected") && sSelectedTextIndex >= 0 && sSelectedTextIndex < static_cast<int>(sTextObjects.size())) {
            sTextObjects.erase(sTextObjects.begin() + sSelectedTextIndex);
            sSelectedTextIndex = -1;
        }

        // List of text objects
        if (ImGui::BeginListBox("##TextObjects", ImVec2(-FLT_MIN, 150.0f))) {
            for (int i = 0; i < static_cast<int>(sTextObjects.size()); ++i) {
                const bool isSelected = (sSelectedTextIndex == i);
                std::string label = sTextObjects[i].name + " [" + sTextObjects[i].fontName + "] [Layer: " + sTextObjects[i].layer + "]";
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    sSelectedTextIndex = i;
                }
            }
            ImGui::EndListBox();
        }

        // Text object properties
        if (sSelectedTextIndex >= 0 && sSelectedTextIndex < static_cast<int>(sTextObjects.size())) {
            ImGui::Separator();
            ImGui::SeparatorText("Text Properties");

            TextObjectData& textObj = sTextObjects[sSelectedTextIndex];

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, 150.0f);  // Wider first column for labels

            // Name
            ImGui::TextUnformatted("Name");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            char nameBuf[64];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", textObj.name.c_str());
            if (ImGui::InputText("##Name", nameBuf, IM_ARRAYSIZE(nameBuf))) {
                textObj.name = nameBuf;
            }
            ImGui::NextColumn();

            // Font selection
            ImGui::TextUnformatted("Font");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##Font", textObj.fontName.c_str())) {
                for (const auto& fontName : sLoadedFonts) {
                    const bool isSelected = (textObj.fontName == fontName);
                    if (ImGui::Selectable(fontName.c_str(), isSelected)) {
                        textObj.fontName = fontName;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::NextColumn();

            // Text content
            ImGui::TextUnformatted("Text");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            char textBuf[256];
            std::snprintf(textBuf, sizeof(textBuf), "%s", textObj.text.c_str());
            if (ImGui::InputText("##Text", textBuf, IM_ARRAYSIZE(textBuf))) {
                textObj.text = textBuf;
            }
            ImGui::NextColumn();

            // NEW: Layer selection
            ImGui::TextUnformatted("Layer");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            {
                // Build list of available layers from scene
                std::vector<std::string> layerNames;
                layerNames.push_back("1"); // Default layer always available
                
                const auto& allLayers = scene.GetAllLayers();
                for (const auto& pair : allLayers) {
                    const std::string& name = pair.first;
                    if (name.empty()) continue;
                    if (std::find(layerNames.begin(), layerNames.end(), name) == layerNames.end()) {
                        layerNames.push_back(name);
                    }
                }
                std::sort(layerNames.begin(), layerNames.end());
                
                if (ImGui::BeginCombo("##Layer", textObj.layer.c_str())) {
                    for (const std::string& name : layerNames) {
                        bool isSelected = (textObj.layer == name);
                        if (ImGui::Selectable(name.c_str(), isSelected)) {
                            textObj.layer = name;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::NextColumn();

            // Position X
            ImGui::TextUnformatted("Position X");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##PosX", &textObj.x, 1.0f);
            ImGui::NextColumn();

            // Position Y
            ImGui::TextUnformatted("Position Y");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##PosY", &textObj.y, 1.0f);
            ImGui::NextColumn();

            // Scale
            ImGui::TextUnformatted("Scale");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##Scale", &textObj.scale, 0.01f, 0.1f, 10.0f);
            ImGui::NextColumn();

            // Rotation
            ImGui::TextUnformatted("Rotation");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SliderFloat("##Rotation", &textObj.rotation, 0.0f, 360.0f, "%.1f deg");
            ImGui::NextColumn();

            // Rotation Mode
            ImGui::TextUnformatted("Rotation Mode");
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            const char* rotModeItems[] = { "Block (Normal)", "Per-Character (Curved)" };
            int currentMode = textObj.useBlockRotation ? 0 : 1;
            if (ImGui::Combo("##RotMode", &currentMode, rotModeItems, IM_ARRAYSIZE(rotModeItems))) {
                textObj.useBlockRotation = (currentMode == 0);
            }
            ImGui::NextColumn();

            // Color
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

            ImGui::Columns(1);
        }

        ImGui::End();
        
        (void)editor;
        (void)scene;
    }
#else
    // Release build: provide no-op implementations
    const std::vector<TextObjectData>& GetTextObjects() {
        static std::vector<TextObjectData> empty;
        return empty;
    }
    
    void SetTextObjects(const std::vector<TextObjectData>& /*textObjects*/) {
        // No-op in Release
    }
    
    void ClearTextObjects() {
        // No-op in Release
    }
    
    std::vector<TextObjectData>& GetMutableTextObjects() {
        static std::vector<TextObjectData> empty;
        return empty;
    }
    
    void DrawFontsPanel(LevelEditor& /*editor*/, Scene& /*scene*/) {
        // Fonts panel disabled in Release builds.
    }
#endif

} // namespace LEPANELFONTS
