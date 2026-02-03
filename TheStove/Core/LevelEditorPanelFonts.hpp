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
        float x, y;
        float scale;
        float rotation;  // Rotation in degrees
        bool useBlockRotation;  // true = block rotation, false = per-character rotation
        float colorR, colorG, colorB, colorA;
        std::string layer{ "1" };  // Layer for rendering order
    };
    
    const std::vector<TextObjectData>& GetTextObjects();
    
    // Functions for level serialization integration
    void SetTextObjects(const std::vector<TextObjectData>& textObjects);
    void ClearTextObjects();
    std::vector<TextObjectData>& GetMutableTextObjects();
}
