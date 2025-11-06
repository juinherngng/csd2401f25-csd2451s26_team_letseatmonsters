// PlayerLogic.hpp
#pragma once
#include "GameObjectLogic.hpp"

class PlayerLogic : public GameObjectLogic {
public:
    using GameObjectLogic::GameObjectLogic; // inherit constructor

    void Update(float dt, Scene& scene, InputManager& input) override;
    std::string GetName() const override { return "PlayerLogic"; }
};
