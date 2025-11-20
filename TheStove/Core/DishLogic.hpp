#pragma once

#include "GameObjectLogic.hpp"
#include "FoodTypes.hpp"

// DishLogic
//  - Represents a completed dish that can be placed on a plate,
//    served to customers, and later marked as eaten.
class DishLogic : public GameObjectLogic
{
public:
    DishLogic(int ownerID, DishType type);

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;

    DishType GetDishType() const { return dishType_; }

    bool IsEaten() const { return isEaten_; }
    void MarkEaten() { isEaten_ = true; }

protected:
    DishType dishType_;
    bool isEaten_;

    std::string GetName() const override { return "DishLogic"; }
};
