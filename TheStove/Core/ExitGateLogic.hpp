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
