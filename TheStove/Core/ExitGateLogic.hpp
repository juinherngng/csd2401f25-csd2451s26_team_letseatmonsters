/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         ExitGateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung

 DESCRIPTION:       Declares the ExitGateLogic component, which defines the world-space
                    exit target for NPCs and provides an optional positional offset.

         All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */


#pragma once
#include "../Core/GameObjectLogic.hpp"
#include "Math.hpp"

class ExitGateLogic : public GameObjectLogic
{
public:
    using GameObjectLogic::GameObjectLogic;

    std::string GetName() const override { return "ExitGateLogic"; }

    // Where NPC should walk to (optionally offset inside the walk area)
    Math::Vector2D GetExitTargetWorld(Scene& scene) const;

    void SetExitOffset(const Math::Vector2D& localOffset) { exitOffset_ = localOffset; }

private:
    Math::Vector2D exitOffset_{ 0.0f, 0.0f };
};
