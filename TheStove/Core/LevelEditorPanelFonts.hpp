/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelFonts.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:       Font management panel for the Level Editor.
                    Allows loading and managing multiple fonts.

        All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>
#include "../Core/FontSystem.hpp"

class LevelEditor;
class Scene;

namespace LEPANELFONTS {
    // UI panel - only handles ImGui interface
    void DrawFontsPanel(LevelEditor& editor, Scene& scene);
    
    // Data access for rendering (read-only)
    struct TextObjectData {
        std::string name;
        std::string fontName;
        std::string text;
        float x{ 0.0f }, y{ 0.0f };
        float scale{ 1.0f };
        float rotation{ 0.0f };  // Rotation in degrees
        bool useBlockRotation{ true };  // true = block rotation, false = per-character rotation
        float colorR{1.0f}, colorG{ 1.0f }, colorB{ 1.0f }, colorA{ 1.0f };
        std::string layer{ "1" };  // Layer for rendering order
        float depth{ 0.0f };  // Depth within layer (higher = rendered on top)
    };
    
    const std::vector<TextObjectData>& GetTextObjects();
    
    // Functions for level serialization integration
    void SetTextObjects(const std::vector<TextObjectData>& textObjects);
    void SetTextObjectsWithScene(const std::vector<TextObjectData>& textObjects, Scene& scene);
    bool SetTextByName(const std::string& name, const std::string& newText);
    void ClearTextObjects();
    std::vector<TextObjectData>& GetMutableTextObjects();
    
    // Selection tracking for hierarchy integration
    int GetSelectedTextIndex();
    void SetSelectedTextIndex(int index);
    
    // Get list of loaded font names
    const std::vector<std::string>& GetLoadedFontNames();

    bool EnsureFontLoaded(const std::string& fontName,
        const std::string& fontPath,
        unsigned int fontSize);

    void EnsureFontsForTextObjectsLoaded(); // loads fonts referenced by sTextObjects
}
