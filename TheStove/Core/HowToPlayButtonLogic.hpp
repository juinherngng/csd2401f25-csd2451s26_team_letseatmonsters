/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            <your name>

 DESCRIPTION:       Declares HowToPlayButtonLogic, which handles the main-menu
                    "How To Play" button: hover texture and showing/hiding a
                    full-screen HowToPlay.png overlay.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include "GameObjectLogic.hpp"

class HowToPlayButtonLogic final : public GameObjectLogic {
public:
    explicit HowToPlayButtonLogic(int ownerID)
        : GameObjectLogic(ownerID) {}

    void Update(float dt, Scene& scene, InputManager& input) override;

private:
    bool initialized_ = false;
    bool hovered_ = false;

    std::string normalTexturePath_;
    std::string hoverTexturePath_;

    int overlayId_ = -1; // ID of spawned HowToPlay overlay, -1 if none
};
