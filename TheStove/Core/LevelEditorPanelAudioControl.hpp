/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         LevelEditorPanelAudioControl.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:       Header for Level Editor Audio Control panel.
                    Provides real-time volume control for playing audio.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

class LevelEditor;
class Scene;

/**
 * @namespace LEPANELAUDIOCONTROL
 * @brief Provides a docked panel for controlling audio playback volume in real-time.
 */
namespace LEPANELAUDIOCONTROL {
    /**
     * @brief Draws the Audio Control panel UI.
     * Shows volume sliders for all loaded audio assets and allows preview playback.
     * 
     * @param editor Reference to the LevelEditor instance.
     * @param scene Reference to the current Scene.
     */
    void DrawAudioControlPanel(LevelEditor& editor, Scene& scene);
}
